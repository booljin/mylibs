#include "shm_wrap.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

CShmWrapper::CShmWrapper() : m_key(0), m_shm_size(0), m_shmid(-1), m_shm_mem(NULL)
{
    m_errmsg[0] = 0;
}

CShmWrapper::~CShmWrapper()
{
    if(m_shm_mem)
        shmdt(m_shm_mem);
    m_shm_mem = NULL;
}

int CShmWrapper::get(key_t key, size_t size, int mode)
{
    m_key = key;
    m_shm_size = size;
    int ret = 0;
    bool exist = false;
    // create share mem
    if((m_shmid = shmget(m_key, m_shm_size, IPC_CREAT | IPC_EXCL | mode)) < 0) // try to create
    {
        if(errno != EEXIST)
        {
            snprintf(m_errmsg, sizeof(m_errmsg), "shmget(IPC_CREAT｜IPC_EXCL, key = 0x%x, size = %ld) %d(%s))",
                m_key, m_shm_size, errno, strerror(errno));
                return ERROR;
        }
        // exist, get
        if((m_shmid = shmget(m_key, 0, mode)) < 0)
        {
            snprintf(m_errmsg, sizeof(m_errmsg), "shmget(key = 0x%x, size = %ld) %d(%s)", m_key, m_shm_size, errno, strerror(errno));
            return ERROR;
        }
        struct shmid_ds stDs;
        ret = shmctl(m_shmid, IPC_STAT, &stDs);
        if(ret != 0)
        {
            snprintf(m_errmsg, sizeof(m_errmsg), "shmctl(IPC_STAT, key = 0x%x, size = %ld) %d(%s)", m_key, m_shm_size, errno, strerror(errno));
            return ERROR;
        }
        m_shm_size = stDs.shm_segsz;
        exist = true;
    }
    if((m_shm_mem = shmat(m_shmid, NULL, 0)) == (void *)-1)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "shmat(%d) %d(%s)", m_shmid, errno, strerror(errno));
        return ERROR;
    }
    if(exist)
        return EXIST;
    else
        return SUCCESS;
}

int CShmWrapper::remove_id(int shmid)
{
    if(shmctl(shmid, IPC_RMID, NULL) < 0)
        return ERROR;
    else
        return SUCCESS;
}

int CShmWrapper::remove(key_t key, int mode)
{
    int shmid = shmget(key, 0, mode);
    if(shmid < 0)
    {
        if(errno != ENOENT)
            return ERROR;
        else
            return SUCCESS;
    }
    return remove_id(shmid);
}

