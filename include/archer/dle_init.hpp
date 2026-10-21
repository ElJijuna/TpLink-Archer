#pragma once
#include "archer/register_mailbox.hpp"
#include <array>
namespace archer {
// Preconditions are lifecycle evidence supplied by a future cold-start coordinator,
// never inferred from packet-buffer register contents. No live repartition API.
struct DleColdStartEvidence {
    bool exclusive_reset_ownership = false;
    bool firmware_cpus_stopped = false;
    bool hci_dma_and_queue_producers_stopped = false;
    bool usb2_scc_selected = false;
};
// RTL8922A USB2 SCC only. Does not initialize HFC, MPDU, dispatcher or USB DMA.
// RegisterBus::pause has no timing contract: caller supplies a >=1us delay.
inline void initialize_usb2_scc_dle(RegisterBus& bus, const DleColdStartEvidence& evidence,
                                    const std::function<void()>& delay_one_us) {
    constexpr std::uint32_t engines = (1u << 26) | (1u << 23);
    constexpr std::uint32_t layout_mask = 0x1fff7f03;
    static_assert(64 * (508 + 4) == 32768, "WDE/PLE boundary");
    static_assert(64 * (508 + 4) + 256 * (2059 + 109) + 2048 == 589824,
                  "USB2 SCC must cover FIFO including reserved tail");
    if (!evidence.exclusive_reset_ownership || !evidence.firmware_cpus_stopped ||
        !evidence.hci_dma_and_queue_producers_stopped || !evidence.usb2_scc_selected ||
        !delay_one_us)
        throw std::runtime_error("DLE requires externally established cold-start prerequisites");
    auto read = [&](unsigned address) {
        const auto value = bus.read(address, 4);
        if (value == 0xeaeaeaea || value == 0xdeadbeef)
            throw std::runtime_error("DLE register inaccessible");
        return value;
    };
    if ((read(0xfc) & 255) != 0x71 || ((read(0xf0) >> 12) & 15) != 1)
        throw std::runtime_error("DLE requires RTL8922A CV1");
    // A running or downloaded firmware lifecycle requires an unimplemented
    // coordinated shutdown, even if its DLE registers happen to look idle.
    if ((read(0x1e0) >> 26) & 15)
        throw std::runtime_error("DLE live firmware transition is not implemented");
    const auto initial = read(0x8400);
    if ((initial & ((1u << 30) | (1u << 29))) != ((1u << 30) | (1u << 29)) ||
        (initial & engines))
        throw std::runtime_error("DLE requires enabled MAC/DMAC and reset DLE engines");
    auto update = [&](unsigned address, unsigned mask, unsigned value) {
        bus.write(address, 4, (read(address) & ~mask) | value);
        if ((read(address) & mask) != value)
            throw std::runtime_error("DLE configuration did not latch");
    };
    // mac.c:rtw89_mac_dle_init; mac_be.c:dle_*_be. Failures leave
    // engines disabled; never restore an earlier partition over partial state.
    try {
        update(0x8400, engines, 0);
        update(0x8404, engines, engines);
        update(0x8c08, layout_mask, 508u << 16);
        update(0x9008, layout_mask, (2059u << 16) | (4u << 8) | 2);
        constexpr std::array<unsigned, 5> wde{{492, 6, 0, 0, 10}};
        constexpr std::array<unsigned, 13> low{{463,0,16,24,7,14,57,0,12,9,1,4,0}};
        constexpr std::array<unsigned, 13> high{{463,0,16,24,1459,1466,1509,0,1464,9,1,1456,0}};
        for (unsigned i = 0; i < wde.size(); ++i)
            update(0x8c40 + 4*i, 0xffffffff, wde[i] | (wde[i] << 16));
        for (unsigned i = 0; i < low.size(); ++i)
            update(0x9040 + 4*i, 0xffffffff, low[i] | (high[i] << 16));
        update(0x8400, engines, engines);
        for (auto address : {0x8d00u, 0x9100u}) {
            bool ready = false;
            for (unsigned elapsed = 0; elapsed <= 2000; ++elapsed) {
                if ((read(address) & 3) == 3) { ready = true; break; }
                if (elapsed < 2000) delay_one_us();
            }
            if (!ready) throw std::runtime_error(address == 0x8d00 ? "WDE readiness timeout" : "PLE readiness timeout");
        }
    } catch (...) {
        auto failure = std::current_exception();
        try { update(0x8400, engines, 0); }
        catch (...) { throw std::runtime_error("DLE failed and engine disable could not be verified; reset required"); }
        std::rethrow_exception(failure);
    }
}
}
