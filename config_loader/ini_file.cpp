#include "ini_file.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define RETURN_DEFAULT(x)\
    {\
        if(ret != NULL)\
            *ret = DEFAULT;\
        return x;\
    }
    
#define RETURN(x)\
    {\
        if(ret != NULL)\
            *ret = SUCCESS;\
        return x;\
    }

void trim_str(char *string)
{
    int len;
    int n, left_start;
    len = strlen(string);
    if(len == 0)
        return;
    
    // trim right first
    for(n = len - 1; n >= 0; --n)
    {
        // trim space and tab
        if(string[n] == ' ' || string[n] == '\t' || string[n] == '\r' || string[n] == '\n')
            string[n] = 0;
        else
            break;
    }
    len = strlen(string);
    if(len == 0)
        return;
    // trim left
    for(n = 0; n < len; ++n)
    {
        if(string[n] != ' ' && string[n] != '\t' && string[n] != '\r' && string[n] != '\n')
            break;
    }
    // no space at the left
    if(n == 0)
        return;
    left_start = n;
    memmove(string, &(string[left_start]), len - left_start + 1);
    memset(string + len - left_start + 1, 0, left_start);
}

CIniFile::CIniFile(const char *ini_file)
{
    FILE *file;
    char *temp_ptr = NULL;
    m_file = m_buffer = NULL;
    m_size = 0;
    struct stat file_stat;
    if(stat(ini_file, &file_stat) != 0)
        return;
    m_size = file_stat.st_size;
    if(m_size <= 0)
        return;
    m_file = m_buffer = (char *)malloc(m_size + 1);
    if(m_buffer == NULL)
        return;
    memset(m_buffer, 0, m_size + 1);
    file = fopen(ini_file, "r");
    fread(m_buffer, m_size, 1, file);
    fclose(file);
    // check utf8 BOM
    if((m_buffer[0]) == '\357' && (m_buffer[1]) == '\273'
        && (m_buffer[2]) == '\277')
    {
        m_buffer += 3;
        m_size -= 3;
    }
    //
    temp_ptr = m_buffer;
    while(temp_ptr < m_buffer + m_size)
    {
        if(*temp_ptr == '\r' || *temp_ptr == '\n')
            *temp_ptr = '\0';
        ++temp_ptr;
    }
    temp_ptr = get_first_line();
    while(temp_ptr != NULL)
    {
        trim_str(temp_ptr);
        adjust_comment(temp_ptr);
        temp_ptr = get_next_line(temp_ptr);
    }
}

CIniFile::~CIniFile()
{
    if(m_file != NULL)
    {
        free(m_file);
    }
}

char * CIniFile::get_str(const char *section, const char *item,
    const char *default_value, char *value, const int value_len, int *ret)
{
    char *section_pos1 = NULL, *section_pos2 = NULL;
    char *item_pos = NULL, *value_pos = NULL;
    char *temp_pos1 = NULL, *temp_pos2 = NULL;
    int n, section_len, item_len;
    
    strncpy(value, default_value, value_len);
    section_len = strlen(section);
    item_len = strlen(item);
    
    if(!valid())
        RETURN_DEFAULT(value);
        
    // locate section first
    temp_pos1 = get_first_line();
    if(section_len == 0)
    {
        section_pos1 = temp_pos1;
    }
    else
    {
        while(temp_pos1 != NULL)
        {
            section_pos1 = strstr(temp_pos1, section);
            if(section_pos1 == NULL)
            {
                section_pos1 = NULL;
                temp_pos1 = get_next_line(temp_pos1);
                continue;
            }
            // find '['
            temp_pos2 = section_pos1 - 1;
            while(temp_pos2 >= temp_pos1)
            {
                if(*temp_pos2 == '[')
                    break;
                else if(*temp_pos2 != ' ' && *temp_pos2 != '\t')
                    break;
                --temp_pos2;
            }
            if(temp_pos2 != temp_pos1 || *temp_pos2 != '[')
            {
                section_pos1 = NULL;
                temp_pos1 = get_next_line(temp_pos1);
                continue;
            }
            // find ']'
            temp_pos2 = section_pos1 + section_len;
            while(*temp_pos2 != '\0')
            {
                if(*temp_pos2 == ']')
                    break;
                else if(*temp_pos2 != ' ' && *temp_pos2 != '\t')
                    break;
                ++temp_pos2;
            }
            if(*temp_pos2 != ']' || *(temp_pos2 + 1) != '\0')
            {
                section_pos1 = NULL;
                temp_pos1 = get_next_line(temp_pos1);
                continue;
            }
            section_pos1 = temp_pos1 = get_next_line(section_pos1);
            break;
        } 
    }
    if(section_pos1 == NULL)
        RETURN_DEFAULT(value);
    //locate the entry of next section
    do
    {
        n = strlen(temp_pos1);
        if(temp_pos1[0] == '[' && temp_pos1[n - 1] == ']')
        {
            section_pos2 = temp_pos1;
            break;
        }
    }
    while((temp_pos1 = get_next_line(temp_pos1)) != NULL);
    // current section has nothing
    if(section_pos1 == section_pos2)
        RETURN_DEFAULT(value);
    //locate item
    temp_pos1 = section_pos1;
    do
    {
        item_pos = strstr(temp_pos1, item);
        if(item_pos == NULL)
        {
            continue;
        }
        // not the beginning of the line
        if(item_pos != temp_pos1)
        {
            continue;
        }
        temp_pos2 = temp_pos1 + item_len;
        while( *temp_pos2 != '\0')
        {
            if(*temp_pos2 != ' ' && *temp_pos2 != '\t')
                break;
            ++temp_pos2;
        }
        // not the exactly matched word
        if(*temp_pos2 != '=')
        {
            continue;
        }
        // extract value
        n = strlen(temp_pos1);
        // skip '='
        ++temp_pos2;
        while(*temp_pos2 == ' ' || *temp_pos2 == '\t')
            ++temp_pos2;
        value_pos = temp_pos2;
        if(*value_pos == '\0')
        {// has no valid value
            RETURN_DEFAULT(value);
        }
        else
        {
            strncpy(value, value_pos, value_len);
            value[value_len - 1] = 0;
            RETURN(value);
        }
    }
    while((temp_pos1 = get_next_line(temp_pos1)) != section_pos2);
    RETURN_DEFAULT(value);;
}

int CIniFile::get_int(const char *section, const char *item,
    const int default_value, int *ret)
{
    char buf[32] = {0};
    int value, t_ret;
    if(!valid())
        RETURN_DEFAULT(default_value);
    get_str(section, item, "", buf, sizeof(buf), &t_ret);
    if(t_ret != 0)
    {
        RETURN_DEFAULT(default_value);
    }
    if(strncmp(buf, "0x", 2) == 0 || strncmp(buf, "0X", 2) == 0)
        value = strtol(buf, NULL, 16);
    else
        value = strtol(buf, NULL, 10);
    RETURN(value);
}

unsigned int CIniFile::get_uint(const char *section, const char *item,
    const unsigned int default_value, int *ret)
{
    char buf[32] = {0};
    unsigned int value;
    int t_ret;
    if(!valid())
        RETURN_DEFAULT(default_value);
    get_str(section, item, "", buf, sizeof(buf), &t_ret);
    if(t_ret != 0)
    {
        RETURN_DEFAULT(default_value);
    }
    if(strncmp(buf, "0x", 2) == 0 || strncmp(buf, "0X", 2) == 0)
        value = strtoul(buf, NULL, 16);
    else
        value = strtoul(buf, NULL, 10);
    RETURN(value);
}

long long CIniFile::get_int64(const char *section, const char *item,
    const long long default_value, int *ret)
{
    char buf[32] = {0};
    long long value;
    int t_ret;
    if(!valid())
        RETURN_DEFAULT(default_value);
    get_str(section, item, "", buf, sizeof(buf), &t_ret);
    if(t_ret != 0)
    {
        RETURN_DEFAULT(default_value);
    }
    if(strncmp(buf, "0x", 2) == 0 || strncmp(buf, "0X", 2) == 0)
        value = strtoll(buf, NULL, 16);
    else
        value = strtoll(buf, NULL, 10);
    RETURN(value);
}

unsigned long long CIniFile::get_uint64(const char *section, const char *item,
    const unsigned long long default_value, int *ret)
{
    char buf[32] = {0};
    unsigned long long value;
    int t_ret;
    if(!valid())
        RETURN_DEFAULT(default_value);
    get_str(section, item, "", buf, sizeof(buf), &t_ret);
    if(t_ret != 0)
    {
        RETURN_DEFAULT(default_value);
    }
    if(strncmp(buf, "0x", 2) == 0 || strncmp(buf, "0X", 2) == 0)
        value = strtoull(buf, NULL, 16);
    else
        value = strtoull(buf, NULL, 10);
    RETURN(value);
}

float CIniFile::get_float(const char *section, const char *item,
    const float default_value, int *ret)
{
    char buf[32] = {0};
    float value;
    int t_ret;
    if(!valid())
        RETURN_DEFAULT(default_value);
    get_str(section, item, "", buf, sizeof(buf), &t_ret);
    if(t_ret != 0)
    {
        RETURN_DEFAULT(default_value);
    }
    value = strtof(buf, NULL);
    RETURN(value);
}

bool CIniFile::valid()
{
    if(m_buffer == NULL || m_size <= 0)
        return false;
    else
        return true;
}

char * CIniFile::get_first_line()
{
    char *ptr = m_buffer;
    while(*ptr == '\0' && ptr < m_buffer + m_size)
        ++ptr;
    if(ptr >= m_buffer + m_size)
        return NULL;
    else
        return ptr;
}

char * CIniFile::get_next_line(char *cur_line)
{
    bool finish_one_line = false;
    char *ptr = cur_line;
    while(ptr < m_buffer + m_size)
    {
        if(!finish_one_line && *ptr == '\0')
            finish_one_line = true;
        else if(finish_one_line && *ptr != '\0')
            break;
        ++ptr;
    }
    if(ptr >= m_buffer + m_size)
        return NULL;
    else
        return ptr;
}

void CIniFile::adjust_comment(char *cur_line)
{
    char *ptr = cur_line;
    int len, n;
    len = strlen(cur_line);
    n = 0;
    while(*ptr != '\0')
    {
        if(*ptr == '#' || (*ptr == ';' && *(ptr + 1) == ';') || (*ptr == '/' && *(ptr + 1) == '/'))
        {
            if(ptr == cur_line || *(ptr - 1) != '\\')
            {
                memset(ptr, 0, len - n);
                break;
            }
            else
            {
                memmove(ptr - 1, ptr, len - n + 1);
                --len;
            }
        }
        else
        {
            ++ptr;
            ++n;
        }
    }
}

#ifdef __DEBUG
void CIniFile::debug()
{
    char *ptr = NULL;
    ptr = get_first_line();
    while(ptr != NULL)
    {
        //printf("%s<\n", ptr);
        ptr = get_next_line(ptr);
    }
}
#endif
