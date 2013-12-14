#include "connect_manager.h"
#include "epoll_wrap.h"
#include <stdio.h>
#include <string.h>

int main()
{
    char buff[100];
    snprintf(buff, sizeof(buff), "hello server\n");
    unsigned len = strlen(buff);
    
    epoll_wrap epoll;
    connect_manager conn_manager;
    epoll.init(&conn_manager, 100);
    if(epoll.create() != 0 || epoll.listen(NULL, 10096))
        printf("--%s \n", epoll.get_err_msg());
    conn_manager.dump();
    while(1)
    {
        if(epoll.do_poll() != 0)
            printf("--%s \n", epoll.get_err_msg());
        for(std::map<int, CONNECT_INFO>::iterator it = conn_manager.m_map.begin();
            it != conn_manager.m_map.end(); ++it)
        {
            if(it->second->type != FD_TYPE_LISTEN)
                if(conn_manager.send_to(it->first, buff, len) != 0)
                    printf("%s\n", conn_manager.get_err_msg());
        }
    }
    return 0;
}
