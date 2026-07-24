#include "coro/context.hpp"
#include "coro/scheduler.hpp"

namespace coro
{
context::context() noexcept
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = this;
    m_engine.init();
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = nullptr;
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            // 如果外部没有注入 stop_cb，那么自行为其添加逻辑
            if (!(this->m_stop_cb))
            {
                m_stop_cb = [&]() { m_job->request_stop(); };
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job->request_stop();
    m_engine.wake_up();
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_add(size_t(register_cnt), memory_order_acq_rel);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_num_wait_task.fetch_add(size_t(register_cnt), memory_order_acq_rel);
}

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    while (!token.stop_requested())
    {
        process_work();
        if (empty_wait_task())
        {
            if (!m_engine.ready())
            {
                // 此处表明 contetx 已执行完所有任务，那么调用停止逻辑
                m_stop_cb();
            }
            else
            {
                continue;
            }
        }

        poll_work();
    }
}

}; // namespace coro
