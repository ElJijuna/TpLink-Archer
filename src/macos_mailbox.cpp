#include "archer/macos_register_bus.hpp"
int main(int argc, char** argv) {
    const std::string mode = argc == 2 ? argv[1] : "--inspect";
    if (argc > 2 || (mode != "--inspect" && mode != "--query-phy-cap" && mode != "--query-with-io-window")) {
        std::cerr << "Usage: archer-mailbox [--inspect | --query-phy-cap | --query-with-io-window]\n"; return 2;
    }
    try {
        MacRegisterBus bus;
        if (mode == "--inspect") {
            for (auto address : {0xfcu,0x1e0u,0x3f0u,0x68u,0x4u,0x74u,
                                 0x7140u,0x7160u,0x7164u,0x8400u,0x10000u})
                (void)bus.read(address,4);
            for (auto address : {0x7160u,0x7164u,0x1f5u}) (void)bus.read(address,1);
        } else {
            const auto cap = mode == "--query-with-io-window"
                ? archer::query_phy_capabilities_with_io_window(bus)
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
