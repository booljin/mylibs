#include "epoll_wrap.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "socket_util.h"
#include "connect_manager.h"

#define RETURN_ERR(errcode, errstr, ...) \
    {\
        m_err_code = errcode;\
        snprintf(m_err_msg, sizeof(m_err_msg), (errstr), __VA_ARGS__);\
        return -1;\
    }

epoll_wrap::epoll_wrap()
{
    m_epoll_fd = -1;
    m_events = NULL;
    m_event_max = m_event_current = 0;
    m_connect_manager = NULL;
}

epoll_wrap::~epoll_wrap()
{
    for(std::vector<int>::iterator it = m_listen_sockets.begin();
        it != m_listen_sockets.end(); ++it)
    {
        close_socket(*it);
    }
    if(m_epoll_fd >= 0)
    {
        close(m_epoll_fd);
    }
    if(m_events != NULL)
    {
        delete[] m_events;
        m_events = NULL;
    }
}

void epoll_wrap::init(connect_manager *connect_manager, unsigned int max_connect)
{
    m_connect_manager = connect_manager;
    m_event_max = max_connect;
}

int epoll_wrap::create()
{
    if(m_connect_manager == NULL)
        RETURN_ERR(-1, "epoll not init when create");
    int ret = ::epoll_create(max_connect);
    if(ret < 0)
    {
        RETURN_ERR(-1, "epoll_create(%u) err:%d:%s", 
            max_connect, errno, strerror(errno));
    }
    m_epoll_fd = ret;
    m_events = (struct epoll_event *)malloc((sizeof(struct epoll_event) * max_connect));
    if(m_events == NULL)
    {
        RETURN_ERR(-1, "malloc events err. max = %d", 
            max_connect);
    }
    m_event_max = max_connect;
    return 0;
}

int epoll_wrap::listen(const char *ip, unsigned short port)
{
    if(m_epoll_fd < 0)
    {
        RETURN_ERR(-1, "epoll not created when listen");
    }
    int temp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(temp_socket < 0 
        || socket_set_noblock(temp_socket) != 0
        || socket_set_reuse(temp_socket) != 0)
    {
        socket_close(temp_socket);
        RETURN_ERR(-1, "listen socket create err : ip %s, port %u",
            ip == NULL ? "0.0.0.0" : ip, port);
    }
    struct sockaddr_in sin;
    if(set_sockaddr(&sin, ip, port) != 0)
    {
        socket_close(temp_socket);
        RETURN_ERR(-1, "bind err : ip %s, port %u",
            ip == NULL ? "0.0.0.0" : ip, port);
    }
    if(::bind(temp_socket, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)
    {
        socket_close(temp_socket);
        RETURN_ERR(-1, "bind err : ip %s, port %u %d, %s",
            ip == NULL ? "0.0.0.0" : ip, port, errno, strerror(errno));
    }
    if(::listen(temp_socket, 128) < 0)
    {
        socket_close(temp_socket);
        RETURN_ERR(-1, "listen err : %d, %s",
            errno, strerror(errno));
    }
    if(add_event(fd, EPOLLIN) != 0)
    {
        socket_close(temp_socket);
        return -1;
    }
    CONNECT_INFO info;
    info.fd = temp_socket;
    info.type = FD_TYPE_LISTEN;
    if(m_connect_manager.insert(fd, info) != 0)
    {
        del_event(temp_socket);
        socket_close(temp_socket);
        RETURN_ERR(-1, "%s", m_connect_manager->get_err_msg());
    }
    return 0;
}

int do_epoll()
{
    int retry_num = 3;
    if(m_epoll_fd < 0 || m_listen_sockets.)
    {
        RETURN_ERR(-1, "epoll not init");
    }
    for(int i = 0; i < retry_num; ++i)
    {
        ret = epoll_wait(m_epoll_fd, m_events, m_event_max, 1000/*暂时默认1s超时*/)；
        if(ret < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            else
            {
                RETURN_ERR(-1, "epoll_wait %d %s", errno, strerror(errno));
            }
        }
        int some_err = 0;
        for(int j = 0; j < ret; ++j)
        {
            int fd = m_events[i].data.fd;
            bool in = (m_events[i].events & EPOLLIN) != 0;
            bool out = (m_events[i].events & EPOLLOUT) != 0;
            CONNECT_INFO *info;
            m_connect_manager.get(fd, &info);
            if(info == NULL)
            {
                snprintf(m_err_msg, sizeof(m_err_msg), "fd not find");
                m_err_code = -1;
                ++some_err;
            }
            if((m_events[i].events & EPOLLERR) 
                || (m_events[i].events & EPOLLHUP))
            {
                snprintf(m_err_msg, sizeof(m_err_msg), "fd %d poll err %s", 
                    &fd, (m_events[i].event & EPOLLERR != 0) ? "ERR" : "HUP");
                m_err_code = -1;
                ++some_err;
                del_event(fd);
                m_connect_manager->remove(fd);
                socket_close(fd);
            }
            if(in)
            {
                if(info->type == FD_TYPE_LISTEN)
                {
                    if(on_listen(fd) != 0)
                    {
                        m_err_code = -1;
                        ++some_err;
                    }
                }
                else
                {
                    if(on_read(info) != 0)
                    {
                        m_err_code = -1;
                        ++some_err;
                    }
                }
            }
            if(out)
            {
                if(on_write(info) != 0)
                {
                    m_err_code = -1;
                    ++some_err;
                }
            }
            break;
        }
    }
    if(ret < 0)
        RETURN_ERR(-1, "epoll_wait more than retry num");
    if(some_err > 0)
        return -1;
    return 0;
}

int epoll_wrap::add_event(int fd, unsigned int flag)
{
    if(m_event_current >= m_event_max)
    {
        RETURN_ERR(-1, "add_event current == max");
    }
    struct epoll_event ev;
    // 不使用EPOLLET
    ev.events |= flag;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if(ret < 0)
        RETURN_ERR(-1,"epoll_ctl(EPOLL_CTL_ADD) %d, %s", errno, strerror(errno));
    ++m_event_current;
    return 0;
}

int epoll_wrap::del_event(int fd)
{
    epoll_event ignored;
    if(epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &ignored) < 0)
        RETURN_ERR(-1, "epoll_ctl(EPOLL_CTL_DEL) %d, %s", errno, strerror(errno));
    if(m_event_current > 0)
        --m_event_current;
    return 0;
}

int epoll_wrap::modify_event(int fd, unsigned int flag)
{
    struct epoll_event ev;
    ev.events |= flag;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    if(ret < 0)
        RETURN_ERR(-1, "epoll_ctl(EPOLL_CTL_MOD) %d, %s", errno, strerror(errno));
    return 0;
}

int epoll_wrap::on_listen(int fd)
{
    int retry_num = 3;
    struct sockaddr_in sin;
    socklen_t sin_len = sizeof(sin);
    for(int i = 0; i < retry_num; ++i)
    {
        ret = ::accept(fd, (sockaddr *)&sin, sin_len);
        if(ret < 0)
        {
            if(errno == EINTR)
                continue;
            RETURN_ERR(-1, "accept err %d, %s", errno, strerror(errno));
        }
        break;
    }
    if(ret < 0)
        RETURN_ERR("too many interrupts when accept");
    if(socket_set_noblock(ret) != 0)
    {
        socket_close(ret);
        RETURN_ERR(-1, "set noblock err");
    }
    if(add_event(ret, EPOLLIN) != 0)
    {
        socket_close(ret);
        return -1;
    }
    CONNECT_INFO info;
    info.fd = ret;
    info.type = FD_TYPE_NORMAL;
    if(m_connect_manager.insert(fd, info) != 0)
    {
        del_event(temp_socket);
        socket_close(temp_socket);
        RETURN_ERR(-1, "%s", m_connect_manager->get_err_msg());
    }
    return 0;
}

int epoll_wrap::on_read(CONNECT_INFO *con)
{
    char a[1024];
    int ret = ::recv(fd, a, 1024, 0);
    if(ret <= 0)
    {
        // ret < 0: EINTR/EAGAIN/EWOULDBLOCK
        printf("recv_end or err\n");
        //del_event(fd);
        //close_socket(fd);
        return -1;
    }
    printf("recv %s\n", a);
    return 0;
}
