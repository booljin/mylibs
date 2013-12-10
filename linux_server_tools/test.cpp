#include "daemon.h"
#include <unistd.h>

int main()
{
    daemon();
    create_lock_file();
    while(1)
        usleep(100);
    return 0;
}