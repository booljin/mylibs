#ifndef __EPOLL_WRAP_H__
#define __EPOLL_WRAP_H__

#include <sys/time.h>
#include <vector>
#include <map>
#include <sys/epoll.h>
//#include <mem>
#include <iostream>
#include <stdio.h>

using namespace std;

// 协议解析接口
class CPacketInterface
{
public:
    static const int RET_OK = 0;
    static const int RET_PACKET_NOT_VALID = -1; // 协议包非法
    static const int RET_PACKET_NEED_MORE_BYTES = -2; // 数据包还没结束
    
    /*
    让server判断分包，on_read中调用
    */
    virtual int get_packet_len(const char *buff, unsigned int buffLen, unsigned int &packetLen) = 0;
    virtual ~CPacketInterface(){}
};

class CControlInterface;

class CEpollWrap
{
public:
    friend class CControlInterface;
    typedef union tagUnSessionID
    {
        unsigned long long id;
        struct
        {
            unsigned int ip;
            unsigned short port;
            unsigned short seq;
        }tcpaddr;
    }UN_SESSION_ID;
    static const int TYPE_LISTEN = 1;
    static const int TYPE_ACCEPT = 2;
    // 可配置
    // 包不要超过大小，fd的writeBuff和readBuff根据这个做基础大小
    // 最大不要超过SIZE_BUFF_LIMIT
    // 对应变量
    // unsigned int m_packSize;
    // unsigned int m_packSizeLimit;
    static const unsigned int SIZE_PACKET_DEFAULT = 8096;
    static const unsigned int SIZE_BUFF_LIMIT = 8096 * 64;
    
    struct LIMIT_STATE
    {
        unsigned int pollinCount; // 总的pollin事件发生次数
        unsigned int lastPollinCount; // 最近一次扫描的事件发生数
        unsigned int long long recvBytes; // 总的接受字节数
        unsigned int long long lastRecvBytes; // 最近一次扫描的接收字节数
        timeval createTime; // 创建时间
        timeval lastActiveTime; // 上次活跃时间
        timeval lastCheckTime; // 上次检查时间
        LIMIT_STATE()
        {
            pollinCount = 0;
            lastPollinCount = 0;
            recvBytes = 0;
            lastRecvBytes = 0;
            memset(&createTime, 0, sizeof(createTime));
            memset(&lastActiveTime, 0, sizeof(lastActiveTime));
            memset(&lastCheckTime, 0, sizeof(lastCheckTime));
        }
        //debug()
    };
    
    struct FDINFO
    {
        int fd;
        int type;
        UN_SESSION_ID sessionID; // 对话id
        LIMIT_STATE state;
        unsigned int timerID;
        CMemBuff writeBuff;
        CMemBuff readBuff;
        //debug()
    };
    type map<int, FDINFO> FDINFO_MAP_TYPE;
    
public:
    CEpollWrap(CPacketInterface *ppack, CControlInterface *pcontrol, timeval *ptimeSource);
    ~CEpollWrap();
    // modify pack limit
    inline void set_pack_buff_size(unsigned int limitSize, unsigned int startSize = SIZE_PACKET_DEFAULT)
    {
        m_packSizeLimit = limitSize;
        if(startSize >= limitSize)
            m_packSize = limitSize;
        else
            m_packSize = startSize;
    }
    // 创建epoll， 使用ET还是LT，默认LT
    // timer跟idleTimeout限制有关，默认开启，关闭后减少内存开销。但idleTimeout无法生效
    int create(unsigned int size, bool useET = false, bool enableTime = false);
    
    // 为客户端连接设置限制
    // checkIntervals检查的时间段长度(秒)
    // limitPollinCount可读时间次数限制
    // limitRecvBytes读取字节数目限制
    // idleTimeoutS空闲超时
    // 任意参数为0=不限制
    // 达到任意条件则关闭连接
    inline void set_session_limit(unsigned int checkIntervalS, unsigned int limitPollinCount,
        unsigned int limitRecvBytes, unsigned int idleTimeoutS)
    {
        m_checkIntervalS = checkIntervalS;
        m_limitPollinCount = limitPollinCount;
        m_limitRecvBytes = limitRecvBytes;
        m_idleTimeoutS = idleTimeoutS;
    }
    // 为server添加listen端口
    int add_listen(string ip, unsigned int port);
    // 开始poll
    int do_poll(unsigned int time_sec, unsigned int time_microsec);
    // 写数据
    // 先尝试直接写fd，如果阻塞则放入buffer中
    int write_packet(int fd, unsigned long long sessionID, const char *packet, unsigned int packetLen);
    // 关闭连接
    int close_fd(int fd, unsigned long long sessionID);
    
    inline FDINFO_MAP_TYPE &getmap()
    {
        return m_mapFD;
    }
    inline const char *errmsg()
    {
        return m_errmsg;
    }
    //debug();
protected:
    int add_event(int fd, unsigned int flag);
    int del_event(int fd);
    int modify_event(int fd, unsigned int flag);
private:
    void on_listen(FDINFO *pinfo);
    void on_write(FDINFO *pinof);
    void on_read(FDINFO *pinfo);
    // 被动关闭
    void close_session(FDINFO *pinfo, bool cancelTimer = true);
    inline unsigned int set_timer(int fd, unsigned int timeS)
    {
        if(!m_ptimer)
            return 0;
        unsigned int timeID = 0;
        if(m_ptimer->set_time_s(timeID, fd, timeS, m_ptimenow) != 0)
        {
            m_ptimer->passerr(m_errmsg, sizeof(m_errmsg));
        }
        return timeID;
    }
    inline void cancel_timer(unsigned int timerID)
    {
        if(!m_ptimer)
            return;
        m_ptimer->del_timer(timeID);
    }
    void timeout();
protected:
    int m_epollFD;
    FDINFO_MAP_TYPE m_mapFD;
    FDINFO_MAP_TYPE::iterator m_mapIt;
    char m_errmsg[512];
    unsigned short m_seq; // 序列号生成
    bool m_useET; // 是否使用epoll et
    epoll_event×m_events; //epoll用的events
    unsigned int m_event_max; // epoll支持的最大连接数
    unsigned int m_event_current; // 当前的event数
    CMemAlloc m_alloc; // 读写缓存分配器
    CPacketInterface *m_pPack;
    CControlInterface *m_pControl;
    unsigned int m_packSize;
    unsigned int m_packSizeLimit;
    timeval *m_ptimenow;
    unsigned int m_checkIntervalS;
    unsigned int m_limitPollinCount;
    unsigned int m_limitRecvBytes;
    unsigned int m_idleTimeoutS;
    CTimerPool<int> *m_ptimer;
    char *m_ptimerMem;
};

class CControlInterface
{
public:
    // 以下接口是accept到的连接的数据回调接口
    /*
    当server收到一个完整的包之后传递给interface
    */
    virtual int pass_packet(int fd, unsigned long long sessionID, const char *packet,
        unsigned int packet_len) = 0;
    // 通知control有新fd
    virtual void on_connect(int fd, unsigned long long sessionID) = 0;
    // 通知control有fd关闭
    virtual void on_close(int fd, unsigned long long sessionID) = 0;
    // 额外的hook，create结束前调用。
    // 可以加入其他fd
    virtual int hook_create(CEpollWrap *host){return 0;}
    // epoll的fd不在host->m_mapFD中或者类型不是预定义的则调用
    // pinfo不存在则为null
    // flag是epoll事件
    virtual void hook_poll(CEpollWrap *host, int fd, unsigned int flag, CEpollWrap::FDINFO *pinfo)
    {}
    virtual ~CControlInterface(){}
};
#endif // __EPOLL_WRAP_H__
