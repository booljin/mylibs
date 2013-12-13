﻿#ifndef __CONNECT_MANAGER_H__
#define __CONNECT_MANAGER_H__

#include <map>

enum
{
    FD_TYPE_LISTEN,
    FD_TYPE_NORMAL,
};

struct CONNECT_INFO
{
    int fd;
    int type;
    char *read_buf;
    char *write_buf;
};

class connect_manager
{
public:
    int insert(int fd, CONNECT_INFO info);
    int remove(int fd);
    int get(int fd, CONNECT_INFO **info);
    
    inline const char *get_err_msg();
    void dump();
private:
    std::map<int, CONNECT_INFO> m_map;
    int m_err_code;
    char m_err_msg[256];
};

const char *connect_manager::get_err_msg()
{
    return m_err_msg;
}

#endif // __CONNECT_MANAGER_H__
