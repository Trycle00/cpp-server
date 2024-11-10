#include "fiber.h"

#include <atomic>
#include <stdint.h>

#include "config.h"
#include "macro.h"

namespace trycle
{

static Logger::ptr logger = GET_LOGGER("system");

static std::atomic<uint32_t> t_fiber_id{0};
static std::atomic<uint64_t> t_fiber_count{0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_thread_fiber;

auto g_fiber_stack_size = Config::lookUp<size_t>("fiber.stack.size", 1024 * 1024, "fiber stack size");

class StackAllocator
{
public:
    static void* alloc(size_t stack_size)
    {
        return malloc(stack_size);
    }
    static void dealloc(void* vp, size_t stack_size)
    {
        free(vp);
    }
};

using StackAlloc = StackAllocator;

Fiber::Fiber()
{
    LOG_FMT_DEBUG(logger, "Fiber init id=%d", m_id);
    m_state = EXEC;
    SetThis(this);
    if (getcontext(&m_ctx))
    {
        ASSERT_M(false, "getcontext error.");
    }
    ++t_fiber_count;
}

Fiber::Fiber(FiberCb cb, size_t stack_size)
    : m_id(++t_fiber_id),
      m_cb(std::move(cb))
{
    LOG_FMT_DEBUG(logger, "Fiber init id=%d", m_id);
    m_stack_size = m_stack_size ? m_stack_size : g_fiber_stack_size->getVal();

    if (getcontext(&m_ctx))
    {
        ASSERT_M(false, "getcontext error.");
    }

    m_stack                = StackAlloc::alloc(m_stack_size);
    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    ++t_fiber_count;
}

Fiber::~Fiber()
{
    --t_fiber_count;
    if (m_stack)
    {
        ASSERT_M(m_state == INIT ||
                     m_state == TERM ||
                     m_state == EXCEPT,
                 "Fiber error in distructor with incorrect state.")
        StackAlloc::dealloc(m_stack, m_stack_size);
    }
    else
    {
        ASSERT_M(!m_cb, "invalid cb");
        ASSERT_M(m_state == EXEC, "invalid state");
        Fiber* cur = t_fiber;
        if (cur == this)
        {
            SetThis(nullptr);
        }
    }

    LOG_FMT_DEBUG(logger, "Fiber destroy id=%d, t_fiber_count=%ld", m_id, t_fiber_count.load());
}

// 重置协程的函数，并设置为INIT或TERM状态
void Fiber::reset(FiberCb cb)
{
    ASSERT_M(m_stack, "Can not reset");
    ASSERT_M(m_state == INIT ||
                 m_state == TERM ||
                 m_state == EXCEPT,
             "invalid state to reset.");
    // SetThis(this);
    m_cb = cb;

    if (getcontext(&m_ctx))
    {
        ASSERT_M(false, "getcontext error");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}
// 切换到协程执行
void Fiber::swap_in()
{
    ASSERT(m_state != EXEC);
    SetThis(this);

    m_state = EXEC;
    if (swapcontext(&t_thread_fiber->m_ctx, &m_ctx))
    {
        ASSERT_M(false, "swapcontext error");
    }
}
// 将协程切换到后台
void Fiber::swap_out()
{
    SetThis(t_thread_fiber.get());
    // m_state = READY;
    if (swapcontext(&m_ctx, &t_thread_fiber->m_ctx))
    {
        ASSERT_M(false, "swapcontext error");
    }
}

// 获取当前协程
Fiber::ptr Fiber::GetThis()
{
    if (t_fiber)
    {
        return t_fiber->shared_from_this();
    }
    // init main fiber
    Fiber::ptr main_fiber(new Fiber());
    ASSERT(main_fiber.get() == t_fiber)
    t_thread_fiber = main_fiber;
    return t_thread_fiber->shared_from_this();
}
// 设置当前协程
void Fiber::SetThis(Fiber* ptr)
{
    t_fiber = ptr;
}

// 将协程切换到后台，并设置为HOLD状态
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    cur->m_state   = HOLD;
    cur->swap_out();
}
// 将协程切换到后台，并设置READY状态
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    cur->m_state   = READY;

    cur->swap_out();
}

// 总协程数
uint64_t Fiber::TotalFibers()
{
    return t_fiber_count;
}

// 协程主方法
void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    std::cout << "11@@@@@@@@@@@@@@@@@@@@" << cur->get_id() << "@@@@@@@@@@@@@@@@@@@@@\n";
    ASSERT(cur);

    try
    {
        std::cout << "cc1@@@@@@@@@@@@@@@@@@@@@" << cur->get_id() << "@@@@@@@@@@@@@@@@@@@@\n";
        cur->m_cb();
        std::cout << "cc2@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
        cur->m_cb    = nullptr;
        cur->m_state = TERM;
    }
    catch (std::exception& e)
    {
        std::cout << "e1@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
        cur->m_state = EXCEPT;
        ASSERT_M(false, "cb exception | " + e.what());
    }
    catch (...)
    {
        std::cout << "e2@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
        cur->m_state = EXCEPT;
        ASSERT_M(false, "cb exception");
    }

    std::cout << "22@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
    LOG_FMT_DEBUG(logger, "@@@@@@@@@@ | id=%d", cur->get_id());
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swap_out();

    ASSERT_M(false, "Never reached!");
}

uint32_t Fiber::GetFiberId()
{
    if (t_fiber)
    {
        return t_fiber->get_id();
    }
    return 0;
}

} // namespace trycle