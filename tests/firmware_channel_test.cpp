#include "archer/firmware_channel.hpp"
#include <cassert>
#include <iostream>
template<class F> static void rejects(F function) {
    bool rejected = false;
    try { function(); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}
int main() {
    using namespace archer;
    auto h2c = encode_h2c(1, 3, 2, 0xab, {0x55,0xaa}, true, true);
    const std::vector<std::uint8_t> expected{0x0d,2,0,0xab,10,0xc0,0,0,0x55,0xaa};
    assert(h2c == expected);
    auto wrapped = wrap_h2c_usb_be(h2c);
    assert(wrapped.size() == 34 && channel_u32(wrapped.data()) == 0x0d00000a);
    assert(std::equal(h2c.begin(), h2c.end(), wrapped.begin() + 24));
    auto boundary = encode_h2c(1,0,0,0,std::vector<std::uint8_t>(480));
    wrapped = wrap_h2c_usb_be(boundary);
    assert(wrapped.size() == 516 && (channel_u32(wrapped.data()) & 0x3fff) == 492);
    assert((channel_u32(wrapped.data() + 28) & 0x3fff) == 488);
    rejects([] { (void)encode_h2c(4,0,0,0,{}); });
    rejects([] { (void)encode_h2c(1,64,0,0,{}); });
    rejects([] { (void)encode_h2c(1,0,256,0,{}); });
    rejects([] { (void)encode_h2c(1,0,0,256,{}); });
    rejects([] { (void)encode_h2c(1,0,0,0,std::vector<std::uint8_t>(16376)); });
    rejects([] { (void)wrap_h2c_usb_be(std::vector<std::uint8_t>(8)); });
    auto c2h = decode_c2h(h2c.data(), h2c.size());
    assert(c2h.category == 1 && c2h.command_class == 3 && c2h.function == 2);
    std::vector<std::uint8_t> transfer(24 + h2c.size());
    channel_put32(transfer.data(), (10u << 24) | h2c.size());
    std::copy(h2c.begin(),h2c.end(),transfer.begin() + 24);
    auto decoded = decode_usb_rx_be(transfer);
    assert(decoded.size() == 1 && decoded[0].c2h->function == 2);
    // Two aggregate records and metadata before the second payload.
    transfer.resize(48 + 40 + 2 + 8 + 8 + 16 + h2c.size());
    channel_put32(transfer.data() + 48, 0x80000000 | (10u << 24) |
        (1u << 14) | (1u << 18) | (1u << 20) | (1u << 22) | h2c.size());
    std::copy(h2c.begin(), h2c.end(), transfer.begin() + 122);
    decoded = decode_usb_rx_be(transfer);
    assert(decoded.size() == 2 && decoded[1].payload_offset == 122);
    rejects([] { (void)decode_usb_rx_be({}); });
    for (std::size_t size = 1; size < 24 + h2c.size(); ++size) {
        std::vector<std::uint8_t> truncated(transfer.begin(), transfer.begin() + size);
        rejects([&] { (void)decode_usb_rx_be(truncated); });
    }
    transfer[24 + 4] = 9;
    rejects([&] { (void)decode_usb_rx_be(transfer); });
    std::cout << "Firmware channel encoding and RX validation: OK\n";
}
