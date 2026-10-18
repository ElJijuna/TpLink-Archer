#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace archer {
struct Endpoint {
    std::uint8_t interface_number, alternate_setting, address, attributes, interval;
    std::uint16_t max_packet_size;
    bool input() const { return (address & 0x80) != 0; }
    bool bulk() const { return (attributes & 3) == 2; }
};
struct Configuration {
    std::uint8_t value;
    std::vector<Endpoint> endpoints;
};
inline std::uint16_t little16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}
// Accept a complete USB configuration descriptor, including interface children.
// Reject malformed lengths before indexing or advancing over any descriptor.
inline Configuration parse_configuration(const std::uint8_t* bytes, std::size_t size) {
    if (!bytes || size < 9 || bytes[0] != 9 || bytes[1] != 2)
        throw std::invalid_argument("Missing USB configuration header");
    const auto total = little16(bytes + 2);
    if (total < 9 || total != size)
        throw std::invalid_argument("USB configuration size mismatch");
    Configuration config{bytes[5], {}};
    std::uint8_t interface = 0, alternate = 0, expected = 0;
    bool have_interface = false;
    std::size_t actual = 0;
    auto check_count = [&] {
        if (have_interface && actual != expected)
            throw std::invalid_argument("USB endpoint count mismatch");
    };
    for (std::size_t offset = 9; offset < size;) {
        if (size - offset < 2 || bytes[offset] < 2 || bytes[offset] > size - offset)
            throw std::invalid_argument("Invalid USB child descriptor length");
        const auto* child = bytes + offset;
        if (child[1] == 4) {
            if (child[0] < 9) throw std::invalid_argument("Short USB interface");
            check_count();
            interface = child[2]; alternate = child[3]; expected = child[4];
            actual = 0; have_interface = true;
        } else if (child[1] == 5) {
            if (child[0] < 7 || !have_interface)
                throw std::invalid_argument("Invalid USB endpoint placement");
            if ((child[2] & 0x0f) == 0 || (child[2] & 0x70))
                throw std::invalid_argument("Invalid USB endpoint address");
            for (const auto& previous : config.endpoints)
                if (previous.interface_number == interface &&
                    previous.alternate_setting == alternate && previous.address == child[2])
                    throw std::invalid_argument("Duplicate USB endpoint");
            config.endpoints.push_back({interface, alternate, child[2], child[3], child[6],
                                       little16(child + 4)});
            ++actual;
        }
        offset += child[0];
    }
    check_count();
    return config;
}
}
