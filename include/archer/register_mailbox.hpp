#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <exception>
#include <optional>
#include <string>

namespace archer {
struct RegisterBus {
    virtual ~RegisterBus() = default;
    virtual std::uint32_t read(std::uint32_t address, unsigned width) = 0;
    virtual void write(std::uint32_t address, unsigned width, std::uint32_t value) = 0;
    virtual void pause() = 0;
};
struct PhysicalCapabilities {
    std::array<std::uint32_t, 4> words;
    unsigned rx_streams, tx_streams, bandwidth_code, protocol_code;
    unsigned tx_antennas, rx_antennas, band_mask, sequence;
};
inline PhysicalCapabilities decode_phy_capabilities(const std::array<std::uint32_t, 4>& w) {
    if ((w[0] & 0x7f) != 3 || ((w[0] >> 8) & 15) != 4)
        throw std::runtime_error("Expected a complete PHY_CAP part-0 response");
    return {w, (w[0] >> 16) & 255, w[1] & 255, w[0] >> 24, (w[1] >> 8) & 255,
            (w[3] >> 8) & 255, (w[3] >> 16) & 255, w[3] >> 24, (w[0] >> 12) & 15};
}
inline PhysicalCapabilities query_phy_capabilities(RegisterBus& bus) {
    // Sources: rtw8922a.c mailbox map; fw.c register handshake; mac.c GET_FEATURE.
    if ((bus.read(0xfc, 4) & 255) != 0x71)
        throw std::runtime_error("Unexpected hardware ID; no mailbox writes allowed");
    if (((bus.read(0x1e0, 4) >> 26) & 15) != 3)
        throw std::runtime_error("Firmware is not in the observed initialized state");
    if (((bus.read(0x3f0, 4) >> 16) & 0x3ff) != 0x220)
        throw std::runtime_error("System is not active; no power transition is implemented");
    const auto h2c_control = bus.read(0x7160, 1);
    const auto c2h_control = bus.read(0x7164, 1);
    if (h2c_control > 1 || c2h_control > 1)
        throw std::runtime_error("Mailbox control is inaccessible or unexpected; no writes allowed");
    if (h2c_control != 0 || c2h_control != 0)
        throw std::runtime_error("Mailbox is busy; refusing to overwrite pending data");
    auto counter = bus.read(0x1f5, 1);
    bus.write(0x7140, 4, 0x103); // GET_FEATURE=3, one 32-bit word, part=0.
    for (auto address : {0x7144u, 0x7148u, 0x714cu}) bus.write(address, 4, 0);
    bus.write(0x1f5, 1, (counter & 0xf0) | ((counter + 1) & 15));
    bus.write(0x7160, 1, 1);
    bool ready = false;
    for (unsigned attempt = 0; attempt < 20; ++attempt) {
        const auto control = bus.read(0x7164, 1);
        if (control > 1) throw std::runtime_error("Unexpected C2H control after request; no reset performed");
        if (control == 1) { ready = true; break; }
        bus.pause();
    }
    if (!ready) throw std::runtime_error("C2H mailbox timeout; no reset or retry performed");
    std::array<std::uint32_t, 4> words{};
    for (unsigned i = 0; i < 4; ++i) words[i] = bus.read(0x7150 + 4 * i, 4);
    // Leave an unrelated/invalid response pending rather than acknowledging it.
    const auto result = decode_phy_capabilities(words);
    bus.write(0x7164, 1, 0);
    counter = bus.read(0x1f5, 1);
    bus.write(0x1f5, 1, (counter & 15) | (((counter >> 4) + 1) & 15) << 4);
    return result;
}
// rtw8922a_pwr_on_func enables HCI_WLAN_IO and waits for IO_ST.
// This diagnostic window restores only the enable bit, preserving other fields.
inline PhysicalCapabilities query_phy_capabilities_with_io_window(RegisterBus& bus) {
    if ((bus.read(0xfc, 4) & 255) != 0x71 ||
        ((bus.read(0x3f0, 4) >> 16) & 0x3ff) != 0x220 ||
        ((bus.read(0x1e0, 4) >> 26) & 15) != 3)
        throw std::runtime_error("Hardware, power or firmware prerequisite failed");
    constexpr std::uint32_t enable = 1u << 28, active = 1u << 31;
    const auto original = bus.read(0x74, 4);
    if (original == 0xeaeaeaea || original == 0xdeadbeef)
        throw std::runtime_error("HCI control register is inaccessible");
    const bool change = !(original & enable);
    std::exception_ptr failure;
    std::optional<PhysicalCapabilities> result;
    try {
        if (change) bus.write(0x74, 4, original | enable);
        bool ready = false;
        for (unsigned attempt = 0; attempt < 20; ++attempt) {
            const auto control = bus.read(0x74, 4);
            if (control == 0xeaeaeaea || control == 0xdeadbeef)
                throw std::runtime_error("HCI control became inaccessible while enabling");
            if (control & active) { ready = true; break; }
            bus.pause();
        }
        if (!ready) throw std::runtime_error("WLAN IO enable did not become ready");
        result = query_phy_capabilities(bus);
    } catch (...) { failure = std::current_exception(); }
    if (change) {
        try {
            const auto current = bus.read(0x74, 4);
            if (current == 0xeaeaeaea || current == 0xdeadbeef)
                throw std::runtime_error("HCI control became inaccessible");
            bus.write(0x74, 4, current & ~enable);
            if (bus.read(0x74, 4) & enable)
                throw std::runtime_error("HCI enable bit did not restore");
        } catch (const std::exception& error) {
            throw std::runtime_error(std::string("WLAN IO restoration failed: ") + error.what());
        }
    }
    if (failure) std::rethrow_exception(failure);
    return *result;
}
}
