#include "archer/usb_descriptors.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
static bool rejects(const std::vector<std::uint8_t>& bytes) {
    try { (void)archer::parse_configuration(bytes.data(), bytes.size()); }
    catch (const std::invalid_argument&) { return true; }
    return false;
}
int main() {
    std::vector<std::uint8_t> bytes{9,2,32,0,1,1,0,0x80,50,
        9,4,0,0,2,255,255,255,0,
        7,5,0x81,2,0,2,0, 7,5,0x02,2,0,2,0};
    auto config = archer::parse_configuration(bytes.data(), bytes.size());
    assert(config.value == 1 && config.endpoints.size() == 2);
    assert(config.endpoints[0].input() && config.endpoints[0].bulk());
    assert(config.endpoints[0].max_packet_size == 512);
    assert(!config.endpoints[1].input());
    for (std::size_t length = 0; length < bytes.size(); ++length)
        assert(rejects({bytes.begin(), bytes.begin() + length}));
    auto malformed = bytes; malformed[18] = 0; assert(rejects(malformed));
    malformed = bytes; malformed[18] = 255; assert(rejects(malformed));
    malformed = bytes; malformed[13] = 3; assert(rejects(malformed));
    malformed = bytes; malformed[27] = 0x81; assert(rejects(malformed));
    malformed = bytes; malformed[20] = 0x80; assert(rejects(malformed));
    // Alternate settings can reuse endpoint addresses.
    bytes.insert(bytes.end(), {9,4,0,1,1,255,255,255,0, 7,5,0x81,2,64,0,0});
    bytes[2] = static_cast<std::uint8_t>(bytes.size());
    config = archer::parse_configuration(bytes.data(), bytes.size());
    assert(config.endpoints[2].alternate_setting == 1);
    std::ifstream file("tests/fixtures/37ad-0104-usb2-config.bin", std::ios::binary);
    assert(file.good());
    std::vector<std::uint8_t> recorded{std::istreambuf_iterator<char>(file), {}};
    config = archer::parse_configuration(recorded.data(), recorded.size());
    assert(config.endpoints.size() == 9);
    unsigned inputs = 0, outputs = 0;
    for (const auto& endpoint : config.endpoints) {
        assert(endpoint.bulk() && endpoint.max_packet_size == 512);
        endpoint.input() ? ++inputs : ++outputs;
    }
    assert(inputs == 2 && outputs == 7);
    std::cout << "USB descriptor validation: OK\n";
}
