#include "control_channel.h"
#include <stdexcept>
#include <utility>

void ControlChannel::run(std::coroutine_handle<> generator)
{
    producer_ = generator;
    next_ = generator;
    while (next_)
        std::exchange(next_, nullptr).resume();
}

bool ControlChannel::DispatchAwaiter::await_suspend(std::coroutine_handle<> producer)
{
    channel_.value_ = value_;
    if (value_ % 2 != 0)
    {
        channel_.log_ << "   [controller] " << value_
            << " is odd -> generator resumes itself\n";
        return false; // не призупиняти: виконання триває
    }

    if (!channel_.printer_)
        throw std::logic_error("coroutine A is not registered in the channel");
    channel_.log_ << "   [controller] " << value_
        << " is even -> control passed to A\n";
    channel_.producer_ = producer;
    channel_.next_ = channel_.printer_;
    return true;
}

void ControlChannel::ReturnAwaiter::await_suspend(std::coroutine_handle<>)
{
    if (!channel_.producer_)
        throw std::logic_error("no generator to return control to");
    channel_.log_ << "   [controller] A is done -> control returned to generator\n";
    channel_.next_ = channel_.producer_;
}