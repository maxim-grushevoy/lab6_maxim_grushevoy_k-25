/*
Програмний проєкт 6, варіант 7.

Сопрограма-генератор породжує випадкові натуральні числа з [1, 256].
Парне число — керування передається сопрограмі A, яка виводить число;
непарне — генератор відновлює власне виконання.
Логіку передачі керування реалізує контролер очікування (control_channel.h).

Збирання (потрібен компілятор з підтримкою C++20):
    MSVC (Microsoft (R) C/C++ Optimizing Compiler Version 19.51)

Запуск та формат вхідних даних (вхідних файлів немає, лише аргументи):
    ./variant7 [кількість [зерно]]
      кількість — ціле від 1 до 1 000 000: скільки чисел згенерувати (типово 10);
      зерно     — ціле від 0 до 4 294 967 295: зерно генератора випадкових
                  чисел (типово випадкове).
Код завершення: 0 — успіх, 1 — помилка виконання, 2 — некоректні аргументи.
*/
#include "control_channel.h"
#include "coroutines.h"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    constexpr std::size_t kDefaultCount = 10;
    constexpr std::size_t kMaxCount = 1'000'000;

    struct Options
    {
        std::size_t count = kDefaultCount;
        std::uint32_t seed = 0;
        bool seed_given = false;
    };

    template <typename T>
    T parse_number(std::string_view text, const char* name)
    {
        T value{};
        const char* end = text.data() + text.size();
        auto [ptr, ec] = std::from_chars(text.data(), end, value);
        if (text.empty() || ec != std::errc{} || ptr != end)
            throw std::invalid_argument(std::string("parameter \"") + name +
                "\" must be a non-negative integer, got \"" +
                std::string(text) + "\"");
        return value;
    }

    // Кидає std::invalid_argument з поясненням, якщо аргументи некоректні.
    Options parse_options(int argc, char* argv[])
    {
        if (argc > 3)
            throw std::invalid_argument("too many arguments");

        Options options;
        if (argc >= 2)
        {
            options.count = parse_number<std::size_t>(argv[1], "count");
            if (options.count < 1 || options.count > kMaxCount)
                throw std::invalid_argument("count must be between 1 and " +
                    std::to_string(kMaxCount));
        }
        if (argc == 3)
        {
            options.seed = parse_number<std::uint32_t>(argv[2], "seed");
            options.seed_given = true;
        }
        return options;
    }

    std::string usage(const std::string& program)
    {
        return "Usage: " + program + " [count [seed]]\n"
            "  count - how many numbers to generate, 1.." + std::to_string(kMaxCount) +
            " (default " + std::to_string(kDefaultCount) + ")\n"
            "  seed  - RNG seed, 0..4294967295 (default: random)";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        Options options = parse_options(argc, argv);
        if (!options.seed_given)
            options.seed = std::random_device{}();

        std::cout << "Variant 7: control transfer between coroutines\n"
            << "Count: " << options.count << ", seed: " << options.seed << "\n\n";

        // Канал оголошено першим, тому він переживе обидві сопрограми.
        ControlChannel channel(std::cout);
        Task printer = printer_a(channel, std::cout);
        channel.bind_printer(printer.get_handle());
        Task producer = number_producer(channel, std::cout, options.count, options.seed);

        channel.run(producer.get_handle()); // керування переходить між сопрограмами

        producer.rethrow_if_failed();
        printer.rethrow_if_failed();
        std::cout << "\nGenerator finished.\n";
        return 0;
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Error: " << e.what() << "\n"
            << usage(argc > 0 ? argv[0] : "variant7") << '\n';
        return 2;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Runtime error: " << e.what() << '\n';
        return 1;
    }
}