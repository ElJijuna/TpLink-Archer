#include "archer/rx_descriptor.hpp"
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: archer-decode RX_TRANSFER_FILE\n"; return 2; }
    try {
        std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
        if (!file || file.tellg() < 0 || file.tellg() > 1024 * 1024)
            throw std::runtime_error("Expected readable RX transfer of at most 1 MiB");
        file.seekg(0);
        std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        if (file.bad()) throw std::runtime_error("RX file read failed");
        auto packets = archer::decode_be_rx_metadata(bytes);
        for (const auto& metadata : packets) {
            const auto& packet = metadata.packet;
            std::cout << "{\"descriptor_offset\":" << packet.descriptor_offset
                      << ",\"payload_offset\":" << packet.payload_offset
                      << ",\"payload_bytes\":" << packet.payload_bytes
                      << ",\"packet_type\":" << packet.packet_type;
            if (packet.packet_type == 0) {
                std::cout << ",\"descriptor_bytes\":" << metadata.descriptor_bytes
                          << ",\"crc_error\":" << (metadata.crc_error ? "true" : "false")
                          << ",\"icv_error\":" << (metadata.icv_error ? "true" : "false")
                          << ",\"rate_code\":" << metadata.rate_code
                          << ",\"bandwidth_code\":" << metadata.bandwidth_code
                          << ",\"header_converted\":";
                if (metadata.header_converted) std::cout << (*metadata.header_converted ? "true" : "false");
                else std::cout << "null";
                std::cout << ",\"fcs_policy\":\"unknown\"";
            }
            if (packet.c2h) std::cout << ",\"category\":" << packet.c2h->category
                                    << ",\"class\":" << packet.c2h->command_class
                                    << ",\"function\":" << packet.c2h->function;
            std::cout << "}\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "decode: " << error.what() << '\n';
        return 2;
    }
}
