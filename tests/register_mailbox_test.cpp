#include "archer/register_mailbox.hpp"
#include <cassert>
#include <map>
#include <tuple>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
struct FakeBus : archer::RegisterBus {
    std::map<std::uint32_t,std::uint32_t> registers{{0xfc,0x80000371}, {0x1e0,0xc000000}, {0x3f0,0x2200000},
        {0x7160,0},{0x7164,0},{0x1f5,0xaf}, {0x7150,0x02021403},
        {0x7154,0x00000402},{0x7158,0},{0x715c,0x03020200}};
    std::vector<std::tuple<std::uint32_t,unsigned,std::uint32_t>> writes;
    bool respond = true;
    bool enable_io = true;
    bool fail_restore = false;
    bool change_unrelated_bit = false;
    unsigned pauses = 0;
    std::uint32_t read(std::uint32_t address, unsigned) override { return registers[address]; }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        if (fail_restore && address == 0x74 && !(value & (1u << 28)))
            throw std::runtime_error("Injected restore transfer failure");
        writes.emplace_back(address,width,value); registers[address] = value;
        if (address == 0x74 && enable_io) {
            if (value & (1u << 28)) registers[address] |= 1u << 31;
            else registers[address] &= ~(1u << 31);
        }
        if (address == 0x7160 && value == 1 && respond) {
            registers[0x7164] = 1;
            if (change_unrelated_bit) registers[0x74] ^= 1;
        }
    }
    void pause() override { ++pauses; }
};
template<class F> static void rejects(F function) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
int main() {
    FakeBus bus;
    auto capabilities = archer::query_phy_capabilities(bus);
    assert(capabilities.rx_streams == 2 && capabilities.tx_streams == 2);
    assert(capabilities.protocol_code == 4 && capabilities.sequence == 1);
    assert(bus.registers[0x7140] == 0x103 && bus.registers[0x7164] == 0);
    assert(bus.registers[0x1f5] == 0xb0); // H2C wrap, preserve C2H nibble; then C2H increment.
    assert(bus.writes.size() == 8);
    for (auto address : {0xfcu,0x1e0u,0x3f0u,0x7160u,0x7164u}) {
        FakeBus invalid;
        invalid.registers[address] = address >= 0x7160 ? 1 : 0;
        rejects([&] { (void)archer::query_phy_capabilities(invalid); });
        assert(invalid.writes.empty());
    }
    FakeBus timeout; timeout.respond = false;
    rejects([&] { (void)archer::query_phy_capabilities(timeout); });
    assert(timeout.pauses == 20 && timeout.writes.size() == 6);
    FakeBus wrong; wrong.registers[0x7150] = 0x402;
    rejects([&] { (void)archer::query_phy_capabilities(wrong); });
    assert(wrong.registers[0x7164] == 1); // Not acknowledged.
    FakeBus inaccessible; inaccessible.registers[0x7160] = 0xea;
    rejects([&] { (void)archer::query_phy_capabilities(inaccessible); });
    assert(inaccessible.writes.empty());
    FakeBus gated; gated.registers[0x74] = 0x1004fd;
    (void)archer::query_phy_capabilities_with_io_window(gated);
    assert(gated.registers[0x74] == 0x1004fd && gated.writes.size() == 10);
    FakeBus blocked; blocked.registers[0x74] = 0x1004fd;
    blocked.registers[0x7160] = 0xea;
    rejects([&] { (void)archer::query_phy_capabilities_with_io_window(blocked); });
    assert(blocked.registers[0x74] == 0x1004fd && blocked.writes.size() == 2);
    FakeBus no_enable; no_enable.registers[0x74] = 0x1004fd; no_enable.enable_io = false;
    rejects([&] { (void)archer::query_phy_capabilities_with_io_window(no_enable); });
    assert(no_enable.registers[0x74] == 0x1004fd && no_enable.writes.size() == 2);
    FakeBus already; already.registers[0x74] = 0x901004fd;
    (void)archer::query_phy_capabilities_with_io_window(already);
    assert(already.registers[0x74] == 0x901004fd && already.writes.size() == 8);
    FakeBus changed; changed.registers[0x74] = 0x1004fd; changed.change_unrelated_bit = true;
    (void)archer::query_phy_capabilities_with_io_window(changed);
    assert(changed.registers[0x74] == 0x1004fc);
    FakeBus restore_failure; restore_failure.registers[0x74] = 0x1004fd;
    restore_failure.fail_restore = true;
    rejects([&] { (void)archer::query_phy_capabilities_with_io_window(restore_failure); });
    assert(restore_failure.registers[0x74] & (1u << 28));
    std::ifstream file("tests/fixtures/phy-cap-part0.bin", std::ios::binary);
    assert(file.good());
    const std::vector<std::uint8_t> recorded{std::istreambuf_iterator<char>(file), {}};
    assert(recorded.size() == 16);
    std::array<std::uint32_t,4> words{};
    for (unsigned i = 0; i < 16; ++i) words[i / 4] |= std::uint32_t(recorded[i]) << ((i % 4) * 8);
    const auto real = archer::decode_phy_capabilities(words);
    assert(real.rx_streams == 2 && real.tx_streams == 2 && real.protocol_code == 4);
    assert(real.tx_antennas == 2 && real.rx_antennas == 2 && real.band_mask == 15);
    rejects([] { (void)archer::decode_phy_capabilities({3,0,0,0}); });
    std::cout << "Register mailbox state and bounds: OK\n";
}
