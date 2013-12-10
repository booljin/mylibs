#include "queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{

    //CQueue queue;
    //queue.init();
    //queue.dump();
    //queue.push("test1:abc", strlen("test1:abc"));
    //queue.dump();
    //queue.push("test2:1234567", strlen("test2:1234566"));
    //queue.dump();
    //char *buf;
    //unsigned int len;
    //queue.pop(&buf, &len);
    //queue.dump();
    //queue.push("test3:!@#$%%^&*)", strlen("test3:!@#$%%^&*)"));
    //queue.dump();
    //queue.push("1", strlen("1"));
    //queue.dump();
    //printf("len = %d, %c", len, buf[len - 1]);

    char *mem = (char *)malloc(50);
    {
        CQueue q1;
        q1.push("a", strlen("a"));
        printf("%s\n", q1.err_msg());
        q1.init(mem, 50);
        q1.push("1234567890", strlen("1234567890"));
        q1.dump();
    }
    {
        queue_data data;
        CQueue q2;
        q2.init(mem, 50);
        q2.push("0123456789", strlen("1234567890"));
        printf("%s\n", q2.err_msg());
        q2.dump();
        q2.pop(data);
        printf("get data %c-%c", data.data[0], data.data[data.len - 1]);
        q2.dump();
        q2.pop(data);
        printf("%s\n", q2.err_msg());
        
    }
    {
        CQueue qq1;
        qq1.init(NULL, 1);
        printf("%s\n", qq1.err_msg());
    }
}