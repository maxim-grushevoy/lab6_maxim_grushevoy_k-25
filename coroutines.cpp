#include "coroutines.h"
#include <random>

Task number_producer(ControlChannel& channel, std::ostream& out,
    std::size_t count, std::uint32_t seed)
{
    std::mt19937 engine(seed);
    std::uniform_int_distribution<int> distribution(kMinValue, kMaxValue);

    for (std::size_t i = 1; i <= count; ++i)
    {
        const int number = distribution(engine);
        out << '#' << i << " generated " << number << '\n';
        co_await channel.dispatch(number);
    }
}

Task printer_a(ControlChannel& channel, std::ostream& out)
{
    while (true)
    {
        out << "   A: " << channel.value() << '\n';
        co_await channel.give_back();
    }
}