#pragma once
#include "control_channel.h"
#include "task.h"
#include <cstddef>
#include <cstdint>
#include <ostream>

constexpr int kMinValue = 1;
constexpr int kMaxValue = 256;

/*
Сопрограма-генератор: породжує count випадкових натуральних чисел
з проміжку [kMinValue, kMaxValue] і по одному віддає їх контролеру.
Про те, куди піде керування, не знає.
*/
Task number_producer(ControlChannel &channel, std::ostream &out,
                     std::size_t count, std::uint32_t seed);

/*
Сопрограма A: виводить число, яке їй передали, і повертає керування.
Працює, доки її не знищить власник.
*/
Task printer_a(ControlChannel &channel, std::ostream &out);
