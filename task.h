#pragma once
#include <coroutine>
#include <exception>
#include <utility>

/*
Власник сопрограми без результату.
Сопрограма створюється призупиненою і ніколи не завершується "сама по собі":
після завершення тіла вона лишається у фінальній точці, а кадр знищує
деструктор власника. Це унеможливлює витоки пам'яті на будь-якому шляху
виконання. Виняток, що не був оброблений у сопрограмі, зберігається і
передається викликачу методом rethrow_if_failed().
*/
class Task
{
public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        std::exception_ptr error;

        Task get_return_object() { return Task{handle::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { error = std::current_exception(); }
    };

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}
    Task &operator=(Task &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            coro_ = std::exchange(other.coro_, nullptr);
        }
        return *this;
    }
    ~Task() { destroy(); }

    // Запустити або відновити сопрограму (якщо вона ще не завершилась).
    void resume()
    {
        if (coro_ && !coro_.done())
            coro_.resume();
    }
    bool done() const noexcept { return !coro_ || coro_.done(); }
    std::coroutine_handle<> get_handle() const noexcept { return coro_; }

    void rethrow_if_failed() const
    {
        if (coro_ && coro_.promise().error)
            std::rethrow_exception(coro_.promise().error);
    }

private:
    explicit Task(handle h) noexcept : coro_(h) {}
    void destroy() noexcept
    {
        if (coro_)
            coro_.destroy();
        coro_ = nullptr;
    }

    handle coro_;
};
