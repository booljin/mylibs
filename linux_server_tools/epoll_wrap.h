#ifndef __EPOLL_WRAP_H__
#define __EPOLL_WRAP_H__
#include <sys/epoll.h>
#include <vector>

// 默认不使用EPOLLET而是EPOLLLT

class connect_manager;
struct CONNECT_INFO;

class epoll_wrap
{
public:
    epoll_wrap();
    ~epoll_wrap();
    void init(connect_manager *connect_manager, unsigned int max_connect);
    int create();
    int listen(const char *ip, unsigned short port);
    int do_poll();
    
    //bool any_error();
    inline const char *get_err_msg();
    //void clear_error();
public:
    int add_event(int fd, unsigned int flag);
    int del_event(int fd);
    int modify_event(int fd, unsigned int flag);
    int send_to(int fd, const char *data, unsigned int len);
    //int close_fd(int fd);
    
private:
    int on_listen(int fd);
    int on_read(CONNECT_INFO *con);
    int on_write(CONNECT_INFO *con);
private:
    int m_epoll_fd;
    struct epoll_event *m_events;
    unsigned int m_event_max;
    unsigned int m_event_current;
    connect_manager *m_connect_manager;
    int m_err_code;
    char m_err_msg[256];
};

const char *epoll_wrap::get_err_msg()
{
    return m_err_msg;
}

#endif // __EPOLL_WRAP_H__
