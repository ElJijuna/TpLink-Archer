#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <optional>
#include <algorithm>

namespace archer {
inline std::uint32_t channel_u32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
inline void channel_put32(std::uint8_t* p, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(value >> (8 * i));
}
// Caller owns command semantics and sequence lifecycle. Encoding performs no I/O.
inline std::vector<std::uint8_t> encode_h2c(unsigned category, unsigned command_class,
    unsigned function, unsigned sequence, const std::vector<std::uint8_t>& payload,
    bool receive_ack = false, bool done_ack = false) {
    if (category > 3 || command_class > 63 || function > 255 || sequence > 255 ||
        payload.size() > 0x3fff - 8)
        throw std::invalid_argument("H2C field or length exceeds wire width");
    std::vector<std::uint8_t> result(8 + payload.size());
    channel_put32(result.data(), category | (command_class << 2) | (function << 8) |
                                      (std::uint32_t(sequence) << 24));
    channel_put32(result.data() + 4, static_cast<std::uint32_t>(result.size()) |
                                  (receive_ack ? 0x4000 : 0) | (done_ack ? 0x8000 : 0));
    std::copy(payload.begin(), payload.end(), result.begin() + 8);
    return result;
}
inline std::vector<std::uint8_t> wrap_h2c_usb_be(const std::vector<std::uint8_t>& h2c) {
    if (h2c.size() < 8 || h2c.size() > 0x3fff ||
        (channel_u32(h2c.data() + 4) & 0x3fff) != h2c.size())
        throw std::invalid_argument("H2C length does not match its header");
    const std::size_t padding = (24 + h2c.size()) % 512 == 0 ? 4 : 0;
    if (h2c.size() + padding > 0x3fff)
        throw std::invalid_argument("Padded H2C exceeds descriptor length field");
    std::vector<std::uint8_t> result(24 + h2c.size() + padding);
    channel_put32(result.data(), (13u << 24) | static_cast<std::uint32_t>(h2c.size() + padding));
    std::copy(h2c.begin(), h2c.end(), result.begin() + 24);
    return result;
}
struct C2hHeader {
    unsigned category, command_class, function, total_length;
};
inline C2hHeader decode_c2h(const std::uint8_t* data, std::size_t size) {
    if (!data || size < 8) throw std::invalid_argument("Truncated C2H header");
    const auto word0 = channel_u32(data), word1 = channel_u32(data + 4);
    const auto length = word1 & 0x3fff;
    if (length < 8 || length != size)
        throw std::invalid_argument("C2H declared length does not match RX packet");
    return {word0 & 3, (word0 >> 2) & 63, (word0 >> 8) & 255, length};
}
struct RxPacket {
    std::size_t descriptor_offset, payload_offset, payload_bytes;
    unsigned packet_type;
    std::optional<C2hHeader> c2h;
};
// BE RX descriptor v2, 16-byte aggregate alignment. Not an Ethernet decoder.
inline std::vector<RxPacket> decode_usb_rx_be(const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) throw std::invalid_argument("Empty USB RX transfer");
    std::vector<RxPacket> result;
    std::size_t cursor = 0;
    while (cursor < bytes.size()) {
        if (bytes.size() - cursor < 24)
            throw std::invalid_argument("Truncated USB RX descriptor");
        const auto word = channel_u32(bytes.data() + cursor);
        const std::size_t descriptor = (word & 0x80000000) ? 40 : 24;
        const auto size = word & 0x3fff;
        const auto type = (word >> 24) & 63;
        const std::size_t metadata = ((word >> 14) & 3) * 2 +
            ((word >> 18) & 3) * 8 + ((word >> 22) & 3) * 8 + ((word >> 20) & 3) * 16;
        const auto payload = cursor + descriptor + metadata;
        if (!size || payload > bytes.size() || size > bytes.size() - payload)
            throw std::invalid_argument("USB RX payload exceeds transfer bounds");
        RxPacket packet{cursor, payload, size, type, std::nullopt};
        if (type == 10) packet.c2h = decode_c2h(bytes.data() + payload, size);
        result.push_back(packet);
        const auto end = payload + size;
        const auto next = cursor + ((end - cursor + 15) & ~std::size_t(15));
        if (next >= bytes.size()) break; // Last record may omit alignment padding.
        cursor = next;
    }
    return result;
}
}
