#include "connect_manager.h"
#include "epoll_wrap.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    char buff[100];
    snprintf(buff, sizeof(buff), "hello server\n");
    unsigned len = strlen(buff) + 1;
    
    epoll_wrap epoll;
    connect_manager conn_manager;
    conn_manager.init(&epoll);
    epoll.init(&conn_manager, 100);
    if(epoll.create() != 0 || epoll.listen(NULL, 10096))
        printf("--%s \n", epoll.get_err_msg());
    conn_manager.dump();
    while(1)
    {
        unsigned int a = 0;
        if(epoll.do_poll() != 0)
            printf("--%s \n", epoll.get_err_msg());
        if(conn_manager.m_map.size() >= 2 && conn_manager.m_map.size() != a)
        {
        for(std::map<int, CONNECT_INFO>::iterator it = conn_manager.m_map.begin();
            it != conn_manager.m_map.end(); ++it)
        {
            if(it->second.type != FD_TYPE_LISTEN)
            {
                printf("send %d\n", it->first);
                if(conn_manager.send_to(it->first, buff, len) != 0)
                    printf("%s\n", conn_manager.get_err_msg());
            }
        }
        }
        a = conn_manager.m_map.size();
        //if(conn_manager.m_map.size() > 1)
        //    conn_manager.dump();
        //sleep(1);
    }
    return 0;
}
