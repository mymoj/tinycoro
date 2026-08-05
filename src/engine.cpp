#include "coro/engine.hpp"
#include "coro/io/io_info.hpp"
#include "coro/task.hpp"

namespace coro::detail
{
using std::memory_order_relaxed;

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.init(config::kEntryLength);
    linfo.egn = this;
    m_submit_io.store(0);
    m_running_io.store(0);
}

auto engine::deinit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_upxy.deinit();
    linfo.egn = nullptr;
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return !m_task_queue.was_empty();
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    return m_upxy.get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    if(m_task_queue.was_empty())
    {
        return {};
    }
    return m_task_queue.pop();
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_task_queue.push(handle);
    m_upxy.write_eventfd(1);
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::notify_stop() noexcept -> void
{
    m_upxy.write_eventfd(1);
}

auto engine::wake_up() noexcept -> void
{
    m_upxy.write_eventfd(1);
}

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    do_io_submit(); // 提交 IO

    auto cnt = m_upxy.wait_eventfd(); // 等待 IO 执行
    if (!wake_by_cqe(cnt))
    {
        return;
    }

    // 取出 IO
    auto num = m_upxy.peek_batch_cqe(m_urc.data(), m_num_io_running.load(std::memory_order_acquire));

    if (num != 0)
    {
        // 处理 IO
        for (int i = 0; i < num; i++)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
        m_num_io_running.fetch_sub(num, std::memory_order_acq_rel);
    }
}

auto engine::do_io_submit() noexcept -> void
{
    // 利用条件判断来决定是否需要调用 io_uring 的提交操作
    if (m_num_io_wait_submit > 0)
    {
        [[CORO_MAYBE_UNUSED]] auto _ = m_upxy.submit();
        m_num_io_running += m_num_io_wait_submit; 
        m_num_io_wait_submit = 0; // io_uring 会一次提交所有 IO
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_submit_io.fetch_add(1, std::memory_order_relaxed);
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return m_submit_io == 0 && m_running_io == 0;
}
}; // namespace coro::detail
