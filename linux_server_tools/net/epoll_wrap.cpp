#include "epoll_wrap.h"
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
//#include "tcpwrap.h"
#include <fcntl.h>
#include <assert.h>

CEpollWrap::CEpoll(CPacketInterface *ppack, CControlInterface *pcontrol, timevel *ptimeSource)
{
    m_epollFD = -1;
    m_errmsg[0] = 0;
    m_seq = 0;
    m_events = NULL;
    m_event_max = 0;
    m_packSize = SIZE_PACKET_DEFAULT;
    m_packSizeLimit = SIZE_BUFF_LIMIT;
    m_pPack = ppack;
    m_pControl = pcontrol;
    m_event_current = 0;
    m_ptimenow = ptimeSource;
    m_checkIntervalS = 0;
    m_limitPollinCount = 0;
    m_limitRecvBytes = 0;
    m_idleTimeoutS = 0;
    m_ptimer = NULL;
    m_ptimerMem = NULL;
}

CEpollWrap::~CEpollWrap()
{
    // 关闭epoll
    if(m_epollFD >= 0)
    {
        close(m_epollFD);
        m_epollFD = -1;
    }
    // 删除事件数组
    if(m_events != NULL)
    {
        delete[] m _events;
        m_events = NULL;
        m_event_max = 0;
    }
    // 关闭所有fd
    for(m_mapIt = m_mapFD.begin(), m_mapIt != m_mapFD.end(); ++m_mapIt)
    {
        close(m_mapIt->first);
    }
    m_mapFD.clear();
    // 关闭timer
    if(m_ptimer != NULL)
    {
        delete m_ptimer;
        m_ptimer = NULL
    }
    if(m_ptimerMem != NULL)
    {
        delete[] m_ptimerMem;
        m_ptimerMem = NULL;
    }
}

int CEpollWrap::create(unsigned int size, bool useET, bool enableTimer)
{
    int ret = epoll_create(size);
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "epoll_create(%u) %d(%s)", size, errno,
            strerror(errno));
        return -1;
    }
    m_epollFD = ret;
    m_useET = useET;
    m_events = new epoll_event[size];
    assert(m_events);
    m_event_max = size;
    if(enableTimer)
    {
        unsigned int memsize = CTimerPool<int>::mem_size(size);
        m_ptimerMem = new char[memsize];
        memset(m_ptimerMem, 0, memsize);
        assert(m_ptimerMem);
        m_ptimer = new CTimerPool<int>(m_ptimerMem, memsize, size, m_ptimenow);
        if(!m_ptimer->valid())
        {
            m_ptimer->passerr(m_errmsg, sizeof(m_errmsg));
            return -1;
        }
    }
    if(m_pControl->hook_create(this) != 0)
        return -1;
    return 0;
}

int CEpollWrap::add_listen(string ip, unsigned int port)
{
    if(m_epollFD < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "epoll not created");
        return -1;
    }
    CTcpListenSocket s;
    int ret = s.init();
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "CTcpListenSocket::init %s", s.errmsg());
        return -1;
    }
    ret = s.set_nonblock();
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "CTcpListenSocket::set_nonblock %s", s.errmsg());
        return -1;
    }
    ret = s.set_reuse_addr();
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "CTcpListenSocket::set_reuse_addr %s", s.errmsg());
        return -1;
    }
    ret = s.listen(ip, port);
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "CTcpListenSocket::listen %s", s.errmsg());
        return -1;
    }
    int fd = s.get_socket();
    // 加入epoll
    if(add_event(fd, EPOLLIN) < 0)
        return -1;
    FDINFO info;
    info.fd = fd;
    info.type = TYPE_LISTEN;
    info.sessionID.tcpaddr.ip = s.getSK()->sin_addr.s_addr;
    info.sessionID.tcpaddr.port = s.getSK()->sin_port;
    info.sessionID.tcpaddr.seq = m_seq++;
    info.state.createTime = *m_ptimenow;
    info.state.lastActiveTime = *m_ptimenow;
    info.timerID = 0;
    m_mapFD[fd] = info;
    s.pass_socket();// 临时对象放弃对socket的管理权
    return 0;
}

int CEpollWrap::do_poll(unsigned int time_sec, unsigned int time_microsec)
{
    if(m_idleTimeoutS > 0)
        timeout();
    if(m_epollFD < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "epoll not created");
        return -1;
    }
    int time_mili = time_sec * 1000 + time_microsec / 1000;
    if(time_mili < 0)
        time_mili = -1;
    unsigned int maxIntr = 3;
    int ret = 0;
    while(maxIntr--)
    {
        ret = epoll_wait(m_epollFD, m_events, m_event_max, time_mili);
        if(ret < 0)
        {
            if(errno == EINTR)
            {
                // 中断，重试
                continue;
            }
            else
            {
                snprintf(m_errmsg, sizeof(m_errmsg), "epoll_wait %d(%s)", errno, strerrno(errno));
                return -1;
            }
        }
        for(int i = 0; i < ret; ++i)
        {
            int fd = m_events[i].data.fd;
            int bin = m_events[i].events & EPOLLIN;
            int bout = m_events[i].events & EPOLLOUT;
            m_mapIt = m_mapFD.find(fd);
            if(m_mapIt == m_mapFD.end())
            {
                m_pControl->hook_poll(this, fd, m_events[i].events, NULL);
                continue;
            }
            FDINOF *pinfo = &(m_mapIt->second);
            if(pinfo->type != TYPE_LISTEN && pinfo->type != TYPE_ACCEPT)
            {
                m_pControl->hook_poll(this, fd, m_events[i].events, pinfo);
                continue;
            }
            if(m_events[i].events & EPOLLERR)
            {
                close_session(pinfo);
                continue;
            }
            if(m_events[i].events & EPOLLHUP)
            {
                close_session(pinfo);
                continue;
            }
            if(bin)
            {
                if(pinfo->type == TYPE_LISTEN)
                {
                    on_listen(pinfo);
                }
                else
                {
                    on_read(pinfo);
                }
            }
            if(bout)
            {
                on_write(pinfo);
            }
        }
        break;
    }
    if(ret < 0)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "epoll_wait intr more then %d", maxIntr);
        return -1;
    }
    return 0;
}

int CEpollWrap::add_event(int fd, unsigned int flag)
{
    if(m_event_current >= m_event_max)
    {
        return -1;
    }
    struct epoll_event ev;
    if(m_useET)
        ev.events = EPOLLET;
    else
        ev.events = 0;
    ev.events |= flag;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_epollFD, EPOLL_CTL_ADD, fd, &ev);
    if(ret < 0)
    {
        return -1;
    }
    ++m_event_current;
    return 0;
}

int CEpollWrap::del_event(int fd)
{
    epoll_event ignored;
    if(epoll_ctl(m_epollFD, EPOLL_CTL_DEL, fd, &ignored) < 0)
    {
        return -1;
    }
    if(m_event_current > 0)
        --m_event_current;
    return 0;
}

int CEpollWrap::modify_event(int fd, unsigned int flag)
{
    struct epoll_evnet ev;
    if(m_useET)
        ev.events = EPOLLET;
    else
        ev.events = 0;
    ev.events != flag;
    ev.data.fd = fd;
    int ret = epoll_ctl(m_epollFD, EPOLL_CTL_MOD, fd, &ev);
    if(ret < 0)
        return -1;
    return ret;
}

void CEpollWrap::on_listen(FDINFO *pinfo)
{
    sockaddr_in sk;
    socklen_t len = sizeof(sk);
    int ret = 0;
    int maxintr = 3;
    while(maxintr--)
    {
        ret = accept(pinfo->fd, (sockaddr *)&sk, &len);
        if(ret < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            // LOG
            return;
        }
        break;
    }
    if(ret < 0)
    {
        return;
    }
    CTcpSocket theSocket;
    theSocket.set_socket(ret);
    if(theSocket.set_nonblock() != 0)
    {
        close(ret);
        return;
    }
    if(add_event(theSocket.pass_socket(), EPOLLIN) < 0)
    {
        close(ret);
        return;
    }
    FDINFO info;
    info.fd = ret;
    info.type = TYPE_ACCEPT;
    info.sessionID.tcpaddr.ip = sk.sin_addr.s_addr;
    info.sessionID.tcpaddr.port = sk.sin_port;
    info.sessionID.tcpaddr.seq = m_seq++;
    info.state.createTime = *m_ptimenow;
    info.state.lastActiveTime = *m_ptimenow;
    if(m_idleTimeoutS > 0)
    {
        info.timerID = set_timer(info.fd, m_idleTimeoutS);
    }
    m_mapFD[info.fd] = info;
    m_pControl->on_connect(info.fd, info.sessionID.id);
}

void CEpollWrap::on_write(FDINFO *pinfo)
{
    if(!pinfo->writeBuff.inited())
    {
        modify_event(pinfo->fd, EPOLLIN);
        return;
    }
    if(pinfo->writeBuff.len() != 0)
    {
        // 可能会有信号打断
        while(true)
        {
            char *buff = pinfo->writeBuff.data();
            unsigned int len = pinfo->writeBuff.len();
            int ret = write(pinfo->fd, buff, len);
            if(ret < 0)
            {
                if(errno == EINTR)
                {
                    continue;
                }
                else if(errno = EPIPE)
                {
                    close_session(pinfo);
                    return;
                }
                else if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    //err
                    break;
                }
            }
            else
            {
                pinfo->writeBuff.mv_head(ret);
                break;
            }
            break;
        }
    }
    // 数据已经写完
    if(pinfo->writeBuff.len() == 0)
    {
        pinfo->writeBuff.destroy();
        modify_event(pinfo->fd, EPOLLIN);
    }
    else
    {
        pinfo->writeBuff.resize();
    }
}

void CEpollWrap::on_read(FDINFO *pinfo)
{
    if(!pinfo->readBuff.inited())
    {
        pinfo->readBuff.init(m_packSize, &m_alloc);
    }
    unsigned int totalRead = 0;
    bool nomore = false;
    while(!nomore)
    {
        if(pinfo->readBuff.left() == 0)
        {
            if(pinfo->readBuff.doubleExt(m_packSizeLimit) != 0)
            {
                pinfo->readBuff.clear();
            }
        }
        char *buff = pinfo->readBuff.data() + pinfo->readBuff.len();
        unsigned int left = pifo->readBuff.left();
        int ret = read(pinfo->fd, buff, left);
        if(ret < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            else if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 读完了
                break;
            }
            else
            {
                // err
                break;
            }
        }
        else if(ret == 0)
        {
            // 对方关闭连接
            close_session(pinfo);
            return;
        }
        else
        {
            totalRead += ret;
            if((unsigned int)ret < left)
            {
                nomore = true;
            }
            pinfo->readBuff.mv_tail(ret);
            // 假设一个数据包中最大100条消息
            for(int t = 0; t < 100; ++t)
            {
                unsigned int packLen = 0;
                int packRet = m_pPack->get_pack_len(pinfo->readBuff.data(), pinfo->readBuff.len(); packLen);
                if(packRet == CPacketInterface::RET_OK)
                {
                    if(pinfo->readBuff.len() < packLen)
                    {
                        // 数据没读完
                        break;
                    }
                    else
                    {
                        // 分包
                        int controlRet = m_pControl->pass_packet(pinfo->fd, pinfo->sessionID.id,
                            pinfo->readBuff.data(), packLen);
                        if(controlRet < 0)
                        {
                            // err
                        }
                        else if(controlRet == 1)
                        {
                            close_fd(pinfo->fd, pinfo->sessionID.id);
                            return;
                        }
                        pinfo->readBuff.mv_head(packLen);
                        if(pinfo->readBuff.len() > 0)
                            pinfo->readBuff.resize();
                        else
                            break;
                    }
                }
                else if(packRet == CPacketInterface::RET_PACKET_NEED_MORE_BYTES)
                {
                    break;
                }
                else
                {
                    pinfo->readBuff.clear();
                    break;
                }
            }
        }
    }
    if(pinfo->readBuff.len() == 0)
    {
        pinfo->readBuff.destroy();
    }
    pinfo->state.pollinCount += 1;
    pinfo->state.recvBytes += totalRead;
    pinfo->state.lastActiveTime = *m_ptimenow;
    if(m_checkIntervalS > 0
        && TIMEVAL_SECOND_INTERVAL_PASSED(pinfo->state.lastActiveTime,
            pinfo->state.lastCheckTime, (int)m_checkIntervalS))
    {
        pinfo->state.lastPollinCount = 1;
        pinfo->state.lastRecvBytes = totalRead;
        pinfo->state.lastCheckTime = pinfo->state.lastActiveTime;
    }
    else
    {
        pinfo->state.lastPollinCount += 1;
        pinfo->state.lastRecvBytes += totalRead;
        if(m_limitPollinCount > 0 && pinfo->state.lastPollinCount >= m_limitPollinCount)
        {
            close_session(pinfo);
            return;
        }
        if(m_limitRecvBytes > 0 && pinfo->state.lastRecvBytes >= m_limitRecvBytes)
        {
            close_session(pinfo);
            return;
        }
    }
}

int CEpollWrap::close_fd(int fd, unsigned long long sessionID)
{
    m_mapIt = m_mapFD.find(fd);
    if(m_mapIt == m_mapFD.end())
    {
        snprintf(m_errmsg, size(m_errmsg), "fd = %d not in map", fd);
        return -1;
    }
    FDINFO *pinfo = &(m_mapIt->second);
    if(sessionID != pinfo->sessionID.id)
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "fd = %d, sessionID(%llu) != packSessionID(%llu)",
            fd, pinfo->sessionID.id, sessionID);
        return -1;
    }
    del_event(fd);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    if(m_idleTimeoutS != 0)
    {
        cancel_timer(pinfo->timerID);
    }
    m_mapFD.erase(fd);
    return 0;
}

int CEpollWrap::write_packet(int fd, unsigned long long sessionID, const char *packet, unsigned int packetLen)
{
    m_mapIt = m_mapFD.find(fd);
    if(m_mapIt == m_mapFD.end())
    {
        snprintf(m_errmsg, sizeof(m_errmsg), "fd = %d not in map", fd);
        return -1;
    }
    FDINFO *pinfo = &(m_mapIt->second);
    if(sessionID != pinfo->sessionID.id)
    {
        sprintf(m_errmsg, sizeof(m_errmsg), "fd = %d sessionID(%llu) != packSessionID(%llu)",
            fd, pinfo->sessionID.id, sessionID);
        return -1;
    }
    const char *buff = packet;
    unsigned int len = packetLen;
    if(!pinfo->writeBuff.inited() || pinfo->writeBuff.len() == 0)
    {
        while(len > 0)
        {
            int ret = write(fd, buff, len);
            if(ret < 0)
            {
                if(errno == EINTR)
                {
                    continue;
                }
                else if(errno == EPIPE)
                {
                    snprintf(m_errmsg, sizeof(m_errmsg), "fd = %d session = %llu write EPIPE closed", fd, sessionID);
                    close_session(pinfo);
                    return -1;
                }
                else if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    snprintf(m_errmsg, sizeof(m_errmsg), "fd = %d session = %llu write %d %s",
                        fd, sessionID, errno, strerror(errno));
                    break;
                }
            }
            else
            {
                buff += ret;
                len -= ret;
            }
        }
    }
    if(len > 0)
    {
        unsigned int usedLen = 0;
        if(pinfo->writeBuff.inited())
            usedLen = pinfo->writeBuff.len();
        unsigned int atLeastLen = len + usedLen;
        if(atLeastLen > m_packSizeLimit)
        {
            snprintf(m_errmsg, sizeof(m_errmsg), "fd = %d session = %llu buffered(%u) and new(%u) > limit("%u")",
                fd, sessionID, usedLen, len, m_packSizeLimit);
            return -1;
        }
        unsigned int suggestLen = atLeastLen + m_packSize;
        if(suggestLen > m_packSizeLimit)
            suggestLen = m_packSizeLimit;
        if(!pinfo->writeBuff.inited())
        {
            pinfo->writeBuff.init(suggestLen, &m_alloc);
        }
        if(pinfo->writeBuff.left() < len)
        {
            pinfo->writeBuff.resize(suggestLen);
        }
        modify_event(fd, EPOLLOUT | EPOLLIN);
        unsigned int wr = pinfo->writeBuff.write(buff,len);
        if(wr != len)
        {
            return -1;
        }
    }
    else
    {
        //error
    }
    return 0;
}

void CEpollWrap::close_session(FDINFO *pinfo, bool cancelTimer)
{
    del_event(pinfo->fd);
    shutdown(pinfo->fd, SHUT_RDWR);
    close(pinfo->fd);
    m_pControl->on_close(pinfo->fd, pinfo->sessionID.id);
    if(m_idleTimeoutS != 0 && cancelTimer)
    {
        cancel_timer(pinfo->timerID);
    }
    m_mapFD.erase(pinfo->fd);
}

void CEpollWrap::timeout()
{
    int fd = 0;
    if(!m_ptimer)
    {
        return;
    }
    vector<unsigned int> vtimerID;
    vector<int> vtimerData;
    vtimerID.reserv(100);
    vtimerData.reserv(100);
    int ret = m_ptimer->check_timer(vtimerID, vtimerData, m_ptimenow);
    if(ret != 0)
    {
        m_ptimer->passerr(m_errmsg, sizeof(m_errmsg));
        return;
    }
    for(unsigned int i = 0; i < vtimerID.size(); ++i)
    {
        fd = vtimerData[i];
        m_mapIt = m_mapFD.find(fd);
        if(m_mapIt != m_mapFD.end())
        {
            FDINFO *pinfo = &(m_mapIt->second);
            if(TIMEVAL_SECOND_INTERVAL_PASSED(*m_ptimenow, pinfo->state.lastActiveTime,
                (int)m_idleTimeoutS))
            {
                close_session(pinfo, false);
            }
            else
            {
                int time = pinfo->state.lastActiveTime.tv_sec
                    + (int)m_idleTimeoutS - (*m_ptimenow).tv_sec;
                if(time <= 0)
                {
                    close_session(pinfo, false);
                }
                else
                {
                    pinfo->timeID = set_timer(fd, time);
                }
            }
        }
    }
}


