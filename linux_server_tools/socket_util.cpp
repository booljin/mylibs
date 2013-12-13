#include "socket_util.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int socket_set_reuse(int fd)
{
    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return -1;
    else
        return 0;
}

int socket_set_noblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
        return -1;
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    else
        return 0;
}

int socket_close(int fd)
{
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return 0;
}

int set_sockaddr(struct sockaddr_in *sockaddr, const char *ip, unsigned short port)
{
    memset(sockaddr, 0, sizeof(sockaddr_in));
    if(ip == NULL)
        sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    else if(!inet_aton(ip, &sockaddr->sin_addr))
        return -1;
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons(port);
    return 0;
}
