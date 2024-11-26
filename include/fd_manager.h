#ifndef TRY_FD_MANAGER_H
#define TRY_FD_MANAGER_H

#include <memory>
#include <stdint.h>
#include <unistd.h>
#include <vector>

#include "singleton.h"
#include "thread.h"

namespace trycle
{

class FdCtx : public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();

    bool init();

    void setTimeout(int type, uint64_t val);
    uint64_t getTimeout(int type);

    bool getIsInit() { return m_isInit; }
    void setIsInit(bool isInit) { m_isInit = isInit; }
    bool getIsSocket() { return m_isSocket; }
    void setIsSocket(bool isSocket) { m_isSocket = isSocket; }
    bool getIsSysNoBlock() { return m_isSysNoBlock; }
    void setIsSysNoBlock(bool isSysNoBlock) { m_isSysNoBlock = isSysNoBlock; }
    bool getIsUserNoBlock() { return m_isUserNoBlock; }
    void setIsUserNoBlock(bool isUserNoBlock) { m_isUserNoBlock = isUserNoBlock; }
    int getFd() { return m_fd; }
    void gstFd(int fd) { m_fd = fd; }
    // uint64_t getRecvTimeout() { return m_recvTimeout; }
    // void getRecvTimeout(uint64_t recvTimeout) { m_recvTimeout = recvTimeout; }
    // uint64_t getSendTimeout() { return m_sendTimeout; }
    // void getSendTimeout(uint64_t sendTimeout) { m_sendTimeout = sendTimeout; }
    bool isClosed() { return m_isClosed; }
    void setClose(bool closed) { m_isClosed = closed; };

private:
    bool m_isInit : 1;
    bool m_isSocket : 1;
    bool m_isSysNoBlock : 1;
    bool m_isUserNoBlock : 1;
    bool m_isClosed : 1;
    int m_fd               = -1;
    uint64_t m_recvTimeout = -1;
    uint64_t m_sendTimeout = -1;
};

class FdManager
{

public:
    typedef std::shared_ptr<FdManager> ptr;
    typedef RWMutex MutexType;

    FdManager();
    ~FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    MutexType m_mutex;
    std::vector<FdCtx::ptr> m_fd_ctx_list;
};

typedef SingletonPtr<FdManager> FdMgr;

} // namespace trycle

#endif // TRY_FD_MANAGER_H
