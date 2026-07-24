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

auto engine::poll_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    // 第1步：提交待处理的 I/O
    auto pending = m_submit_io.exchange(0, std::memory_order_relaxed);
    if (pending > 0) {
        auto submitted = m_upxy.submit();
        m_running_io.fetch_add(submitted, std::memory_order_relaxed);
    }

    // 第2步：有运行中的 I/O → 等完成 + 处理
    if (m_running_io.load(std::memory_order_relaxed) > 0) {
        m_upxy.wait_eventfd();                          // 阻塞等 I/O 完成
        auto num = m_upxy.peek_batch_cqe(m_urc.data(), config::kQueCap);
        for (int i = 0; i < num; i++) {
            handle_cqe_entry(m_urc[i]);                  // 处理每个 CQE
        }
        m_upxy.cq_advance(num);                          // 标记已处理
        m_running_io.fetch_sub(num, std::memory_order_relaxed);
    } else {
        // 第3步：没有 I/O → 阻塞等任务到来
        m_upxy.wait_eventfd();
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
