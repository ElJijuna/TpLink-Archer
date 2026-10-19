#include "archer/firmware_channel.hpp"
#include "archer/command_tracker.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
template<class F> static void rejects(F function) {
    bool rejected = false;
    try { function(); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}
int main() {
    using namespace archer;
    const auto diagnostic = diagnostic_log_disable(0);
    assert(diagnostic.size() == 44 && channel_u32(diagnostic.data()) == 0x0d000014);
    assert(channel_u32(diagnostic.data()+28) == 0xc014);
    assert(diagnostic[32] == 4 && diagnostic[33] == 2);
    std::ifstream capture("tests/fixtures/log-disable-receive-ack-usb.bin", std::ios::binary);
    assert(capture.good());
    const std::vector<std::uint8_t> recorded{std::istreambuf_iterator<char>(capture), {}};
    const auto actual = decode_usb_rx_be(recorded);
    assert(actual.size() == 1 && actual[0].payload_bytes == 12);
    const auto ack = decode_receive_ack(recorded.data()+actual[0].payload_offset, 12);
    assert(ack.category == 1 && ack.command_class == 0 && ack.function == 0);
    assert(ack.sequence == 0);
    CommandTracker track(1, 0, 0, 0);
    assert(track.observe(recorded.data()+24, 12) && track.received && !track.completion);
    CommandTracker other(1, 0, 0, 1);
    assert(!other.observe(recorded.data()+24, 12) && !other.received);
    auto done = std::vector<std::uint8_t>(recorded.begin()+24, recorded.end());
    done[1] = 1; // Synthetic DONE_ACK; physical fixture remains REC_ACK.
    assert(track.observe(done.data(), done.size()) && track.completion == 0);
    assert(track.observe(done.data(), done.size())); // Identical duplicate.
    done[10] = 2;
    rejects([&] { (void)track.observe(done.data(), done.size()); });
    CommandTracker failed(1, 0, 0, 0);
    assert(failed.observe(done.data(), done.size()) && failed.completion == 2);
    assert(!failed.received); // Completion can precede receipt.
    done[11] = 1;
    assert(!track.observe(done.data(), done.size())); // Stale/foreign sequence.
    CommandTracker physical(1, 0, 0, 1);
    for (unsigned round = 0; round < 3; ++round) {
        std::ifstream event("tests/fixtures/log-disable-stage11-round-" + std::to_string(round) + ".bin", std::ios::binary);
        assert(event.good());
        const std::vector<std::uint8_t> frame{std::istreambuf_iterator<char>(event), {}};
        const auto packets = decode_usb_rx_be(frame);
        assert(packets.size() == 1);
        assert(physical.observe(frame.data()+packets[0].payload_offset, packets[0].payload_bytes) == (round != 0));
        assert(physical.received == (round >= 1));
        assert(physical.completion.has_value() == (round == 2));
    }
    assert(physical.completion == 0);
    auto bad_ack = std::vector<std::uint8_t>(recorded.begin()+24, recorded.end());
    bad_ack[1] = 1;
    rejects([&] { (void)decode_receive_ack(bad_ack.data(), bad_ack.size()); });
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
