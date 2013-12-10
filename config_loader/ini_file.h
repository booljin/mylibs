/*
auther: booljin
date:   2013.11.22

1，支持基于段落的名值对型配置文件
2，段落名必须用"[]"括住
3，如果没有段落名，则归入匿名段落
4，匿名段落的所以配置项必须出现在任何具名段落之前，否则将不会被查找到
5，支持"#",";;","//"3种注释风格。如果配置中需要用到这几个字符，可以使用转义字符'\'转义

例:
item = 10   ;;匿名段
[SectionA]
item = abc  //SectionA段
[SectionB]
pi = 3.14   
s = a\#b    #s="a#b"
*/
#ifndef __INI_FILE_H__
#define __INI_FILE_H__
#include <stdlib.h>
class CIniFile
{
public:
    const static int SUCCESS = 0;
    const static int ERROR = -1;
    const static int DEFAULT = -401;
public:
    /*
    load ini file into buffer
    */
    CIniFile(const char *ini_file);
    
    /*
    release buffer
    */
    ~CIniFile();
    
    /*
    read the value from special section
    */
    char * get_str(const char *section, const char *item,
        const char *default_value, char *value, const int value_len, int *ret = NULL);
    int get_int(const char *section, const char *item, const int default_value, int *ret = NULL);
    unsigned int get_uint(const char *section, const char *item, const unsigned int default_value, int *ret = NULL);
    long long get_int64(const char *section, const char *item, long long default_value, int *ret = NULL);
    unsigned long long get_uint64(const char *section, const char *item, unsigned long long default_value, int *ret = NULL);
    float get_float(const char *section, const char *item, const float default_value, int *ret = NULL);
    /*
    read the value from default section
    */
    inline char * get_str(const char *item, const char *default_value, char *value, const int value_len, int *ret = NULL)
    {
        return get_str("", item, default_value, value, value_len, ret);
    }
    inline int get_int(const char *item, const int default_value, int *ret = NULL)
    {
        return get_int("", item, default_value, ret);
    }
    inline unsigned int get_uint(const char *item, const unsigned int default_value, int *ret = NULL)
    {
        return get_uint("", item, default_value, ret);
    }
    inline long long get_int64(const char *item, long long default_value, int *ret = NULL)
    {
        return get_int64("", item, default_value, ret);
    }
    inline unsigned long long get_uint64(const char *item, unsigned long long default_value, int *ret = NULL)
    {
        return get_uint64("", item, default_value, ret);
    }
    inline float get_float(const char *item, const float default_value, int *ret = NULL)
    {
        return get_float("", item, default_value, ret);
    }
    /*
    to see if the loader successfully load ini file into buffer
    */
    bool valid();
private:
    char * get_first_line();
    char * get_next_line(char *cur_line);
    // comment is "#" ";;" "//"
    void adjust_comment(char *cur_line);
private:
    char *m_file;
    char *m_buffer;
    int m_size;
#ifdef __DEBUG
public:
    void debug();
#endif
};

#endif //__INI_FILE_H__