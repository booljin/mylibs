#include "connect_manager.h"
#include "epoll_wrap.h"
#include <stdio.h>

#define RETURN_ERR(errcode, errstr, ...) \
    {\
        m_err_code = errcode;\
        snprintf(m_err_msg, sizeof(m_err_msg), (errstr), ##__VA_ARGS__);\
        return -1;\
    }

connect_manager::connect_manager()
{
    m_net_core = NULL;
}
    
void connect_manager::init(epoll_wrap *net_core)
{
    m_net_core = net_core;
}

int connect_manager::insert(int fd, CONNECT_INFO info)
{
    std::map<int, CONNECT_INFO>::iterator it = m_map.find(fd);
    if(it != m_map.end())
        RETURN_ERR(-1, "fd exist when insert into connect_manager %d", fd);
    m_map[fd] = info;
    return 0;
}

int connect_manager::remove(int fd)
{
    std::map<int, CONNECT_INFO>::iterator it = m_map.find(fd);
    if(it == m_map.end())
        RETURN_ERR(-1, "fd not exist when remove from connect_manager %d", fd);
    m_map.erase(fd);
    return 0;
}

int connect_manager::get(int fd, CONNECT_INFO **info)
{
    std::map<int, CONNECT_INFO>::iterator it = m_map.find(fd);
    if(it == m_map.end())
    {
        *info = NULL;
        RETURN_ERR(-1, "fd not exist when remove from connect_manager %d", fd);
    }
    *info = &(it->second);
    return 0;
}

void connect_manager::dump()
{
    for(std::map<int, CONNECT_INFO>::iterator it = m_map.begin();
        it != m_map.end(); ++it)
    {
        printf("fd  :  %d, type : %d\n", it->second.fd, it->second.type);
    }
}

int connect_manager::send_to(int fd, const char *buff, unsigned int len)
{
    if(m_net_core == NULL)
        RETURN_ERR(-1, "connect manager has no net core");
    std::map<int, CONNECT_INFO>::iterator it = m_map.find(fd);
    if(it == m_map.end())
    {
        RETURN_ERR(-1, "fd %d not find while send", fd);
    }
    if(m_net_core->send_to(fd, buff, len) != 0)
    {
        RETURN_ERR(-1, "%s", m_net_core->get_err_msg());
    }
    return 0;
}
