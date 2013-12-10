#include "shm_wrap.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
    CShmWrapper shm;
    int ret = shm.get(0x1234, 100);
    if(ret == CShmWrapper::SUCCESS)
        printf("create success\n");
    else if(ret == CShmWrapper::EXIST)
        printf("exist\n");
    else
        printf("error?\n");
    while(1)
        usleep(1000);
    return 0;
}

