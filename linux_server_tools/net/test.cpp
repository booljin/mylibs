#include "connect_manager.h"
#include "epoll_wrap.h"

int main()
{
    epoll_wrap epoll;
    connect_manager conn_manager;
    epoll.init(&conn_manager, 100);
    if(epoll.create() != 0 || epoll.listen(NULL, 10086))
        printf("%s \n", epoll.get_err_msg());
    while(1)
    {
        if(epoll.do_epoll() != 0)
            printf("%s \n", epoll.get_err_msg());
    }
    return 0;
}