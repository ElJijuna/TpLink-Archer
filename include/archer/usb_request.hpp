#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace archer {
// Protocol encoding derived from Linux rtw89/usb.c and usb.h; no I/O.
// This does not establish that a register is safe to read or write.
enum class Direction { read, write };
inline std::array<std::uint8_t, 8> register_request(
    Direction direction, std::uint32_t address, std::uint16_t length) {
    if (address > 0xffffff || (length != 1 && length != 2 && length != 4) ||
        address + length - 1 > 0xffffff)
        throw std::invalid_argument("Expected 24-bit register range and width 1, 2 or 4");
    return {static_cast<std::uint8_t>(direction == Direction::read ? 0xc0 : 0x40),
            0x05, static_cast<std::uint8_t>(address),
            static_cast<std::uint8_t>(address >> 8),
            static_cast<std::uint8_t>(address >> 16), 0,
            static_cast<std::uint8_t>(length), 0};
}
}
