#include "queue.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define QUEUE_PAGE 5 /*内存分配按页计算，一页为4k*/
#define QUEUE_MAX_LEN 4194304   /*允许最大内存为4M*/
#define ALIGN(x) ((x) + QUEUE_PAGE - 1) / QUEUE_PAGE * QUEUE_PAGE

#define QUEUEMAGIC "QUEUE~!@#$%^&*("

enum e_queue_err
{
    QUEUE_ERR_UNKNOW = 0,
    QUEUE_ERR_NOT_INIT,
    QUEUE_ERR_NO_ENOUGH_MEM,        // 内存分配不足
    QUEUE_ERR_MEM_IS_TOO_SMALL,     // 预先分配的内存太小
    QUEUE_ERR_QUEUE_FULL,           // 队列已满
    QUEUE_ERR_NO_ENOUGH_DATA,       // 试图读取的数据多余已有数据
};

queue_data::queue_data():data(NULL), len(0)
{
}

queue_data::~queue_data()
{
    if(data != NULL)
        free(data);
}

CQueue::CQueue()
{
    m_head = NULL;
    m_data = NULL;
    m_queue_size = 0;
    m_limit_mem = 0;
    m_err_code = 0;
}

CQueue::~CQueue()
{
    if(m_head != NULL)
    {
        if(m_limit_mem == 0)
            free((void *)m_head);
            if(m_limit_mem == 0)
            printf("lalala\n");
    }
}

int CQueue::init()
{
    char *mem = (char *)malloc(ALIGN(sizeof(struct queue_head) + QUEUE_PAGE)/*QUEUE_PAGE*/);
    if(mem == NULL)
    {
        m_err_code = QUEUE_ERR_NO_ENOUGH_MEM;
        snprintf(m_err_msg, sizeof(m_err_msg), "has no enough memory when malloc. need %u bytes", QUEUE_PAGE);
        return -1;
    }
    format(mem, /*QUEUE_PAGE*/ALIGN(sizeof(struct queue_head) + QUEUE_PAGE));
    return 0;
}

int CQueue::init(char *mem, unsigned int size)
{
    if(size <= (sizeof(struct queue_head) + QUEUE_RESERVED_SPACE))
    {
        m_err_code = QUEUE_ERR_MEM_IS_TOO_SMALL;
        snprintf(m_err_msg, sizeof(m_err_msg), "the memory pre-allocated must bigger then %d", sizeof(struct queue_head) + QUEUE_RESERVED_SPACE);
        return -1;
    }
    m_limit_mem = 1;
    struct queue_head *temp_head = (struct queue_head *)mem;
    if(strncmp(temp_head->magic, QUEUEMAGIC, sizeof(temp_head->magic)) == 0)
    { // 内存块可能有有效数据
        if(temp_head->mem_size == size
            && (temp_head->start + sizeof(struct queue_head)) < size
            && (temp_head->end + sizeof(struct queue_head)) < size)
        {
            m_head = temp_head;
            m_data = (char *)(m_head + 1);
            m_queue_size = m_head->mem_size - sizeof(struct queue_head);
            return 0;
        }
    }
    format(mem, size);
    return 0;
}

int CQueue::push(char *data, unsigned int len)
{
    if(m_head == NULL)
    {
        m_err_code = QUEUE_ERR_NOT_INIT;
        snprintf(m_err_msg, sizeof(m_err_msg), "this queue is not initialized yet");
        return -1;
    }
    if(left_len() < len + sizeof(len))
    {
        if(m_limit_mem == 0)
        {
            if(grow(len + sizeof(len)) != 0)
                return -1;
        }
        else
        {
            m_err_code = QUEUE_ERR_QUEUE_FULL;
            snprintf(m_err_msg, sizeof(m_err_msg), "the queue is full");
            return -1;
        }
    }
    unsigned int end = m_head->end;
    write((char *)&len, sizeof(len), end);
    write(data, len, end + sizeof(len));
    m_head->end = (end + sizeof(len) + len) % m_queue_size;
    return 0;
}

int CQueue::pop(queue_data& data)
{
    if(data.data != NULL)
    {
        free(data.data);
        data.data = NULL;
        data.len = 0;
    }
    if(m_head == NULL)
    {
        m_err_code = QUEUE_ERR_NOT_INIT;
        snprintf(m_err_msg, sizeof(m_err_msg), "this queue is not initialized yet");
        return -1;
    }
    if(used_len() < sizeof(data.len))
    {
        m_err_code = QUEUE_ERR_NO_ENOUGH_DATA;
        snprintf(m_err_msg, sizeof(m_err_msg), "the queue is empty");
        return -1;
    }
    unsigned int start = m_head->start;
    read((char *)&data.len, sizeof(data.len), start);
    if(used_len() < sizeof(data.len) + data.len)
    {
        m_err_code = QUEUE_ERR_NO_ENOUGH_DATA;
        snprintf(m_err_msg, sizeof(m_err_msg), "the queue is empty");
        return -1;
    }
    data.data = (char *)malloc(data.len);
    if(data.data == NULL)
    {
        m_err_code = QUEUE_ERR_NO_ENOUGH_MEM;
        snprintf(m_err_msg, sizeof(m_err_msg), "has no enough memory when malloc. need %u bytes", data.len);
        return -1;
    }
    read(data.data, data.len, start + sizeof(data.len));
    m_head->start = (start + sizeof(data.len) + data.len) % m_queue_size;
    return 0;
}

void CQueue::format(char *mem, unsigned int size)
{
    memset(mem, 0, size);
    m_head = (struct queue_head *)mem;
    strncpy(m_head->magic, QUEUEMAGIC, sizeof(m_head->magic));
    m_head->mem_size = size;
    m_head->start = m_head->end = 0;
    m_data = mem + sizeof(struct queue_head);
    m_queue_size = m_head->mem_size - sizeof(struct queue_head);
}

int CQueue::grow(unsigned int size)
{
    if(m_limit_mem != 0)
        return -1;
    unsigned int new_size = ALIGN(m_head->mem_size + size);
    char *new_mem = (char *)malloc(new_size);
    if(new_mem == NULL)
    {
        m_err_code = QUEUE_ERR_NO_ENOUGH_MEM;
        snprintf(m_err_msg, sizeof(m_err_msg), "has no enough memory when malloc. need %u bytes", new_mem);
        return -1;
    }
    struct queue_head *t_head = m_head;
    char *t_data = m_data;
    unsigned int t_len = m_queue_size;
    m_head = (struct queue_head *)new_mem;
    m_data = new_mem + sizeof(struct queue_head);
    m_queue_size = new_size - sizeof(struct queue_head);
    memcpy(m_head, t_head, sizeof(struct queue_head));
    m_head->mem_size = new_size;
    if(t_head->start == t_head->end)
    {
        //assert(false);
        m_head->start = m_head->end = 0;
    }
    else if(t_head->start < t_head->end)
    {
        memcpy(m_data, t_data + t_head->start, t_head->end - t_head->start);
        m_head->start = 0;
        m_head->end = t_head->end - t_head->start;
    }
    else
    {
        memcpy(m_data, t_data + t_head->start, t_len - t_head->start);
        memcpy(m_data + (t_len - t_head->start), t_data, t_head->end);
        m_head->start = 0;
        m_head->end = t_len - (t_head->start - t_head->end);
    }
    free(t_head);
    return 0;
}

int CQueue::write(char *data, unsigned int len, unsigned int end)
{
    unsigned int t_end = end % m_queue_size;
    unsigned int left = m_queue_size - t_end;
    if(left >= len)
    {
        memcpy(m_data + t_end, data, len);
    }
    else
    {
        memcpy(m_data + t_end, data, left);
        memcpy(m_data, data + left, len - left);
    }
    return 0;
}

int CQueue::read(char *buf, unsigned int len, unsigned int start)
{
    unsigned int t_start = start % m_queue_size;
    unsigned int left = m_queue_size - t_start;
    if(left >= len)
    {
        memcpy(buf, m_data + t_start, len);
    }
    else
    {
        memcpy(buf, m_data + t_start, left);
        memcpy(buf + left, m_data, len - left);
    }
    return 0;
}

void CQueue::dump()
{
    char temp_dump[10];
    printf("dump queue:\n");
    if(m_head == NULL)
    {
        printf("\tnot init\n");
        return;
    }
    printf("\tmem_size = %u, queue_size = %u\n", m_head->mem_size, m_queue_size);
    printf("\tused len = %u, start = %u, end = %u\n", used_len(), m_head->start, m_head->end);
    int idx = 0;
    int t_start = m_head->start;
    unsigned int len = 0;
    unsigned int read_len = 0;
    while(1)
    {
        if(used_len() < (read_len + sizeof(len)))
            break;
        read((char *)&len, sizeof(len), t_start + read_len);
        read_len += sizeof(len);
        if(used_len() < read_len + len)
            break;
        read(temp_dump, sizeof(temp_dump), t_start + read_len);
        idx++;
        printf("\t\t%d\t|len = %d:\t", idx, len);
        for(int i = 0; i < 10; ++i)
            printf("%c", temp_dump[i]);
        printf("\n");
        read_len += len;
    }
    if(read_len != used_len())
        printf("\tdirty data!!\n");
}

const char *CQueue::err_msg()
{
    if(m_err_code == QUEUE_ERR_UNKNOW)
        snprintf(m_err_msg, sizeof(m_err_msg), "no err or unknow err");
    return m_err_msg;
}