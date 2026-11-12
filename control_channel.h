#pragma once
#include <coroutine>
#include <ostream>

/*
Контролер очікування (передачі керування).

Уся логіка вибору, кому передати керування, зосереджена тут, в awaiter-ах;
код сопрограм про неї нічого не знає:
  - парне число   -> генератор призупиняється, керування передається
                     сопрограмі A;
  - непарне число -> генератор не призупиняється, тобто відновлює
                     власне виконання.
Сопрограма A, закінчивши роботу з числом, через give_back() повертає
керування генератору.

Сам перехід виконує цикл run() ("батут"): awaiter лише записує, кого
відновити наступним. Завдяки цьому стек викликів не росте, скільки б разів
керування не переходило туди й назад, і програма не залежить від того, чи
виконує компілятор оптимізацію хвостових викликів (симетричної передачі).

Час життя: канал має існувати довше за всі сопрограми, що його використовують.
*/
class ControlChannel
{
public:
    explicit ControlChannel(std::ostream &log) : log_(log) {}
    ControlChannel(const ControlChannel &) = delete;
    ControlChannel &operator=(const ControlChannel &) = delete;

    // Зареєструвати сопрограму A (отримувача парних чисел).
    void bind_printer(std::coroutine_handle<> printer) noexcept { printer_ = printer; }

    // Запустити generator і виконувати передачі керування, доки хтось
    // не завершиться, не призначивши наступника.
    void run(std::coroutine_handle<> generator);

    // Останнє передане число.
    int value() const noexcept { return value_; }

    class DispatchAwaiter
    {
    public:
        DispatchAwaiter(ControlChannel &channel, int value) noexcept
            : channel_(channel), value_(value) {}
        bool await_ready() const noexcept { return false; }
        // true — призупинитись (керування піде до A), false — продовжити.
        bool await_suspend(std::coroutine_handle<> producer);
        void await_resume() const noexcept {}

    private:
        ControlChannel &channel_;
        int value_;
    };

    class ReturnAwaiter
    {
    public:
        explicit ReturnAwaiter(ControlChannel &channel) noexcept : channel_(channel) {}
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<>);
        void await_resume() const noexcept {}

    private:
        ControlChannel &channel_;
    };

    // co_await channel.dispatch(n): передати число n контролеру.
    DispatchAwaiter dispatch(int value) noexcept { return {*this, value}; }
    // co_await channel.give_back(): повернути керування генератору.
    ReturnAwaiter give_back() noexcept { return ReturnAwaiter{*this}; }

private:
    std::ostream &log_;
    int value_ = 0;
    std::coroutine_handle<> producer_;
    std::coroutine_handle<> printer_;
    std::coroutine_handle<> next_; // кого відновити наступним
};
