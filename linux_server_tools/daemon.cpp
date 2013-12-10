#include "daemon.h"

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int daemon()
{
    pid_t pid = fork();
    if(pid == -1)
        return -1;
    else if(pid != 0)
        exit(0);
    if(setsid() == -1)
        return -1;
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if(pid != 0)
        exit(1);
    umask(0);
    return 0;
}

int create_lock_file()
{
    int pid_fd;
    struct flock flock_info;
    int ret;
    if((pid_fd = open("/home/bool/test.lock", O_RDWR | O_CREAT, 644)) == -1)
        return -2;
    flock_info.l_type = F_WRLCK;
    flock_info.l_whence = SEEK_SET;
    flock_info.l_start = 0;
    flock_info.l_len = 1;
    ret = fcntl(pid_fd, F_SETLK, &flock_info);
    if(ret == -1)
        return -3;
    char buf[16] = {0};
    snprintf(buf, sizeof(buf), "%d\n", getpid());
    ftruncate(pid_fd, 0);
    write(pid_fd, buf, strlen(buf));
    return 0;
}