/*
author  : booljin
date    : 2013/12/04

每个数据块的起始为一个unsigned int类型的长度信息
暂时不考虑多线程
*/
#ifndef __QUEUE_H__
#define __QUEUE_H__

#define QUEUE_RESERVED_SPACE 1  /*队列收尾不能重叠，所以预留一字节空间*/

struct queue_head
{
    char magic[16];
    unsigned int mem_size;  // queue的总大小
    unsigned int start;     // queue数据的起点
    unsigned int end;       // queue数据的终点
};

struct queue_data
{
    char *data;
    unsigned int len;
public:
    queue_data();
    ~queue_data();
};

class CQueue
{
public:
    CQueue();
    ~CQueue();
    int init();
    int init(char *mem, unsigned int size);
    int push(char *data, unsigned int len);
    // pop中分配内存，使用完毕后必须自己释放
    int pop(queue_data& data);
public:
    void dump();
    inline unsigned int err_code();
    const char *err_msg();
private:
    void format(char *mem, unsigned int size);
    int grow(unsigned int size);
    // 这两个函数不实际修改相应的职能，所以依靠传入的end和start来精确定位
    // 更新指针的事情交由push和pop等公有函数处理
    int write(char *data, unsigned int len, unsigned int end);
    int read(char *buf, unsigned int len, unsigned int start);
    
private:
    inline unsigned int left_len();
    inline unsigned int used_len();
private:
    struct queue_head *m_head;
    char *m_data;
    unsigned int m_queue_size;
    unsigned int m_limit_mem;       // 指示这个队列是否建立在受限内存上（不分配，不回收，不增长）
    unsigned int m_err_code;        // 这样就不支持多线程了
    char m_err_msg[128];
};

unsigned int CQueue::err_code()
{
    return m_err_code;
}

unsigned int CQueue::used_len()
{
    if(m_head->start == m_head->end)
        return 0;
    else if(m_head->start < m_head->end)
        return m_head->end - m_head->start;
    else
        return m_queue_size - (m_head->start - m_head->end);
}

unsigned int CQueue::left_len()
{
    return m_queue_size - used_len() - QUEUE_RESERVED_SPACE;
}


#endif // __QUEUE_H__
