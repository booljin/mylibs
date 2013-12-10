/*
auther: booljin
date:   2013.11.22

1��֧�ֻ��ڶ������ֵ���������ļ�
2��������������"[]"��ס
3�����û�ж��������������������
4������������������������������κξ�������֮ǰ�����򽫲��ᱻ���ҵ�
5��֧��"#",";;","//"3��ע�ͷ�������������Ҫ�õ��⼸���ַ�������ʹ��ת���ַ�'\'ת��

��:
item = 10   ;;������
[SectionA]
item = abc  //SectionA��
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