#include "fd_manager.h"

#include <sys/stat.h>

#include "hook.h"

namespace trycle
{

/**
 * ============================================================================
 * FdCtx 类的实现
 * ============================================================================
 */
FdCtx::FdCtx(int fd)
    : m_fd(fd),
      m_isInit(false),
      m_isSocket(false),
      m_isSysNoBlock(false),
      m_isUserNoBlock(false),
      m_recvTimeout(-1),
      m_sendTimeout(-1)
{
    init();
}

FdCtx::~FdCtx()
{
}

bool FdCtx::init()
{
    if (m_isInit)
    {
        return false;
    }

    // linux 文件描述符状态变量
    struct stat fd_stat;
    // 文件是否已经关闭
    if (fstat(m_fd, &fd_stat) == -1)
    {
        // 全非，不达到条件
        m_isInit   = false;
        m_isSocket = false;
    }
    else
    {
        m_isInit = true;
        // 判定是否为 socket 文件描述符
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if (m_isSocket)
    {
        int flags = fcntl_f(m_fd, F_GETFL);
        // 如果是 socket 并且是阻塞形式
        if (!(flags & O_NONBLOCK))
        {
            // 将文件标识设置为 非阻塞形式，后续交由 hook 作异步处理
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_isSysNoBlock = true;
    }
    else
    {
        m_isSysNoBlock = false;
    }

    m_isUserNoBlock = false;
    m_recvTimeout   = -1;
    m_sendTimeout   = -1;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t val)
{
    if (type == SO_RCVTIMEO)
    {
        m_recvTimeout = val;
    }
    else
    {
        m_sendTimeout = val;
    }
}

uint64_t FdCtx::getTimeout(int type)
{
    if (type == SO_RCVTIMEO)
    {
        return m_recvTimeout;
    }
    else
    {
        return m_sendTimeout;
    }
}

/**
 * ============================================================================
 * FdManager 类的实现
 * ============================================================================
 */
FdManager::FdManager()
{
    m_fd_ctx_list.resize(64);
}

FdManager::~FdManager()
{
}

FdCtx::ptr FdManager::get(int fd, bool auto_create)
{
    if (fd == -1)
    {
        return nullptr;
    }
    MutexType::ReadLock lock(&m_mutex);
    if (m_fd_ctx_list.size() <= fd)
    {
        if (!auto_create)
        {
            return nullptr;
        }
    }
    else
    {
        if (m_fd_ctx_list[fd] || !auto_create)
        {
            return m_fd_ctx_list[fd];
        }
    }
    lock.unlock();

    MutexType::WriteLock lock2(&m_mutex);
    FdCtx::ptr fd_ctx(new FdCtx(fd));
    if (fd >= m_fd_ctx_list.size())
    {
        m_fd_ctx_list.resize(m_fd_ctx_list.size() * 1.5);
    }
    m_fd_ctx_list[fd] = fd_ctx;
    return fd_ctx;
}

void FdManager::del(int fd)
{
    MutexType::WriteLock lock(&m_mutex);
    if (m_fd_ctx_list.size() <= fd)
    {
        return;
    }

    m_fd_ctx_list[fd].reset();
}

} // namespace trycle