/**
 * @file mutex.hpp
 * @author JiahuiWang
 * @brief lab4d
 * @version 1.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <type_traits>

#include "coro/comp/mutex_guard.hpp"
#include "coro/detail/types.hpp"

namespace coro
{
/**
 * @brief Welcome to tinycoro lab4d, in this part you will build the basic coroutine
 * synchronization component----mutex by modifing mutex.hpp and mutex.cpp.
 * Please ensure you have read the document of lab4d.
 *
 * @warning You should carefully consider whether each implementation should be thread-safe.
 *
 * You should follow the rules below in this part:
 *
 * @note The location marked by todo is where you must add code, but you can also add code anywhere
 * you want, such as function and class definitions, even member variables.
 *
 * @note lab4 and lab5 are free designed lab, leave the interfaces that the test case will use,
 * and then, enjoy yourself!
 */

class context;

// TODO[lab4d]: This mutex is an example to make complie success,
// You should delete it and add your implementation, I don't care what you do,
// but keep the member function and construct function's declaration same with example.
using detail::awaiter_ptr;

class mutex
{
public:
    struct mutex_awaiter
    {
        mutex_awaiter(context& ctx, mutex& mtx) noexcept : m_ctx(ctx), m_mtx(mtx) {}

        constexpr auto await_ready() noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool;

        auto await_resume() noexcept -> void;

        auto register_lock() noexcept -> bool;

        virtual auto resume() noexcept -> void;

        context&                m_ctx;
        mutex&                  m_mtx;
        mutex_awaiter*          m_next{nullptr};
        std::coroutine_handle<> m_await_coro{nullptr};
    };

    struct mutex_guard_awaiter : public mutex_awaiter
    {
        using guard_type = detail::lock_guard<mutex>;
        using mutex_awaiter::mutex_awaiter;

        auto await_resume() noexcept -> guard_type
        {
            mutex_awaiter::await_resume();
            return guard_type(m_mtx);
        }
    };

public:
    mutex() noexcept : m_state(nolocked), m_resume_list_head(nullptr) {}
    ~mutex() noexcept { assert(m_state.load(std::memory_order_acquire) == mutex::nolocked); }

    auto try_lock() noexcept -> bool;

    auto lock() noexcept -> mutex_awaiter;

    auto unlock() noexcept -> void;

    auto lock_guard() noexcept -> mutex_guard_awaiter;

private:
    friend mutex_awaiter;

    // make locked_no_waiting = 0 make state change easier
    inline static awaiter_ptr nolocked          = reinterpret_cast<awaiter_ptr>(1);
    inline static awaiter_ptr locked_no_waiting = 0; // nullptr
    std::atomic<awaiter_ptr>  m_state;
    awaiter_ptr               m_resume_list_head;
};

}; // namespace coro
