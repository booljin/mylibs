#ifndef __SOCKET_UTIL_H__
#define __SOCKET_UTIL_H__
#include <netinet/in.h>

int socket_set_reuse(int fd);
int socket_set_noblock(int fd);
int socket_close(int fd);

int set_sockaddr(struct sockaddr_in *sockaddr, const char *ip, unsigned short port);

#endif //__SOCKET_UTIL_H__

