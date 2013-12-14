#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1)
    {
        printf("errno = %d\n", errno);
        return 0;
    }
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(10096);
    sa.sin_addr.s_addr = inet_addr("192.168.127.128");
    if(connect(socketfd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        printf("connect %d\n", errno);
        close(socketfd);
        return 0;
    }
    //char msg[4] = {'a', 'b', 'c', '\0'};
    //int len = sizeof(msg);
    //int ret = send(socketfd, msg, len, 0);
    //if(ret < 0)
    //{
    //    printf("send err %d\n", errno);
    //    close(socketfd);
    //    return 0;
    //}
    char buff[100];
    int ret = ::recv(socketfd, buff, 100, 0);
    if(ret <= 0)
    {
        printf("ret = %d\n",ret);
        return 0;
    }
    else
        printf("%s\n", buff);
    
    close(socketfd);
    return 0;
}
