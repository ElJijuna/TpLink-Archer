#include "archer/macos_register_bus.hpp"
int main(int argc, char** argv) {
    const std::string mode = argc == 2 ? argv[1] : "--inspect";
    if (argc > 2 || (mode != "--inspect" && mode != "--query-phy-cap" && mode != "--query-with-io-window" && mode != "--inspect-transport")) {
        std::cerr << "Usage: archer-mailbox [--inspect | --query-phy-cap | --query-with-io-window | --inspect-transport]\n"; return 2;
    }
    try {
        MacRegisterBus bus;
        if (mode == "--inspect") {
            for (auto address : {0xfcu,0x1e0u,0x3f0u,0x68u,0x4u,0x74u,
                                 0x7140u,0x7160u,0x7164u,0x8400u,0x10000u})
                (void)bus.read(address,4);
            for (auto address : {0x7160u,0x7164u,0x1f5u}) (void)bus.read(address,1);
        } else {
            const auto cap = (mode == "--query-with-io-window" || mode == "--inspect-transport")
                ? archer::query_phy_capabilities_with_io_window(bus, [&] {
                    if (mode != "--inspect-transport") return;
                    // Observations only: no DMA/queue configuration writes.
                    for (auto address : {0x6000u, 0x6004u, 0x7880u, 0x8400u,
                                         0x8404u, 0x8420u, 0x8430u, 0x8434u,
                                         0x8c08u, 0x8c10u, 0x9008u, 0x9010u,
                                         0xb000u, 0xb010u, 0x10000u, 0x8d00u, 0x9100u})
                        (void)bus.read(address, 4);
                    (void)bus.read(0x000a, 2);
                    for (auto address : {0x4078u, 0x407au, 0x407cu}) (void)bus.read(address, 2);
                    for (unsigned i = 0; i < 5; ++i) (void)bus.read(0x8c40 + 4*i, 4);
                    for (unsigned i = 0; i < 13; ++i) (void)bus.read(0x9040 + 4*i, 4);
                })
                : archer::query_phy_capabilities(bus);
            std::cout << "{\"result\":\"phy_cap_part0\",\"rx_streams\":" << cap.rx_streams
                      << ",\"tx_streams\":" << cap.tx_streams
                      << ",\"bandwidth_code\":" << cap.bandwidth_code
                      << ",\"protocol_code\":" << cap.protocol_code
                      << ",\"tx_antennas\":" << cap.tx_antennas
                      << ",\"rx_antennas\":" << cap.rx_antennas
                      << ",\"band_mask\":" << cap.band_mask
                      << ",\"sequence\":" << cap.sequence << "}\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "mailbox: " << error.what() << '\n'; return 2;
    }
}
