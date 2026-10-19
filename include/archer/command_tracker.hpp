#pragma once
#include "archer/firmware_channel.hpp"
namespace archer {
// One immutable command identity per bounded diagnostic session. No retransmission.
class CommandTracker {
    unsigned category_, class_, function_, sequence_;
public:
    bool received = false;
    std::optional<unsigned> completion;
    CommandTracker(unsigned category, unsigned command_class, unsigned function, unsigned sequence)
        : category_(category), class_(command_class), function_(function), sequence_(sequence) {
        if (category > 3 || command_class > 63 || function > 255 || sequence > 255)
            throw std::invalid_argument("Invalid command identity");
    }
    bool observe(const std::uint8_t* data, std::size_t size) {
        const auto h = decode_c2h(data, size);
        if (h.category != 1 || h.command_class != 0 || h.function > 1) return false;
        if (size != 12) throw std::invalid_argument("Invalid ACK length");
        const auto word = channel_u32(data + 8);
        const auto sequence = h.function == 0 ? (word >> 16) & 255 : word >> 24;
        if ((word & 3) != category_ || ((word >> 2) & 63) != class_ ||
            ((word >> 8) & 255) != function_ || sequence != sequence_) return false;
        if (h.function == 0) received = true;
        else {
            const auto code = (word >> 16) & 255;
            if (completion && *completion != code)
                throw std::invalid_argument("Conflicting completion ACKs");
            completion = code;
        }
        return true;
    }
};
}
