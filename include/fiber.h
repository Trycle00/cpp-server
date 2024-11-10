#ifndef TRY_FIBER_H
#define TRY_FIBER_H

#include <functional>
#include <memory>
#include <ucontext.h>

namespace trycle
{

// 协程类
class Fiber : public std::enable_shared_from_this<Fiber>
{
    enum State
    {
        INIT,
        EXEC,
        HOLD,
        TERM,
        READY,
        EXCEPT,
    };

public:
    typedef std::shared_ptr<Fiber> ptr;
    typedef std::function<void()> FiberCb;

    Fiber(FiberCb cb, size_t stack_size = 0);
    ~Fiber();

    // 重置协程的函数，并设置为INIT或TERM状态
    void reset(FiberCb cb);
    // 切换到协程执行
    void swap_in();
    // 将协程切换到后台
    void swap_out();

    uint32_t get_id() { return m_id; }

public:
    // 获取当前协程
    static Fiber::ptr GetThis();
    // 设置当前协程
    static void SetThis(Fiber* ptr);
    // 将协程切换到后台，并设置为HOLD状态
    static void YieldToHold();
    // 将协程切换到后台，并设置READY状态
    static void YieldToReady();
    // 总协程数
    static uint64_t TotalFibers();
    // 协程主方法
    static void MainFunc();
    // 获取协程id
    static uint32_t GetFiberId();

private:
    Fiber();

private:
    uint32_t m_id       = 0;
    size_t m_stack_size = 0;
    State m_state       = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    FiberCb m_cb;
};

} // namespace trycle

#endif // TRY_FIBER_H