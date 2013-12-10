#ifndef __SHM_WRAP_H__
#define __SHM_WRAP_H__

#include <sys/ipc.h>
#include <sys/shm.h>

class CShmWrapper
{
public:
    enum RET
    {
        EXIST = 1,
        SUCCESS = 0,
        ERROR = -1,
    };
public:
    CShmWrapper();
    ~CShmWrapper();
    
    /*
    shmget + attach
    return SHM_EXIST(已经存在) success(新共享内存) ERROR(出错)
    已经存在的shm,调用get_shm_size获取实际的大小
    */
    int get(key_t key, size_t size, int mode = 0666);
    
    // 返回size
    inline size_t get_shm_size()
    {
        return m_shm_size;
    }
    inline int get_shm_id()
    {
        return m_shmid;
    }
    /*
    调用系统的shmctl RMID,如果仍然有attach中的进程，实际还是可以访问该id的
    再去getshm相同的key会得到不同的id
    */
    static int remove_id(int shmid);
    static int remove(key_t key, int mode = 0644);
    // 返回attach过的内存
    inline void *get_mem()
    {
        return m_shm_mem;
    }
    inline const char *errmsg()
    {
        return m_errmsg;
    }
private:
    key_t m_key;
    size_t m_shm_size;
    int m_shmid;
    void *m_shm_mem;
    char m_errmsg[256];
};


#endif // __SHM_WRAP_H__
