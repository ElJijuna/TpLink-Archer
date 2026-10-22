#include "archer/rx_descriptor.hpp"
#include "archer/beacon.hpp"
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) { std::cerr << "Usage: archer-decode RX_TRANSFER_FILE [--fcs-appended|--fcs-stripped]\n"; return 2; }
    try {
        auto fcs=archer::RxFcsPolicy::unknown;
        if(argc==3){
            const std::string option=argv[2];
            if(option=="--fcs-appended")fcs=archer::RxFcsPolicy::appended;
            else if(option=="--fcs-stripped")fcs=archer::RxFcsPolicy::stripped;
            else throw std::invalid_argument("Unknown FCS option");
        }
        std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
        if (!file || file.tellg() < 0 || file.tellg() > 1024 * 1024)
            throw std::runtime_error("Expected readable RX transfer of at most 1 MiB");
        file.seekg(0);
        std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        if (file.bad()) throw std::runtime_error("RX file read failed");
        auto packets = archer::decode_be_rx_metadata(bytes,fcs);
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
                std::cout << ",\"fcs_policy\":\"" << (fcs==archer::RxFcsPolicy::unknown ? "unknown" :
                    fcs==archer::RxFcsPolicy::appended ? "appended" : "stripped") << "\"";
                if(fcs!=archer::RxFcsPolicy::unknown && metadata.normal_80211_candidate()){
                    try {
                        const auto beacon=archer::decode_rx_beacon(bytes,metadata);
                        std::cout << ",\"management_kind\":\"" << (beacon.probe_response ? "probe_response" : "beacon")
                                  << "\",\"ssid_bytes\":" << beacon.ssid.size()
                                  << ",\"interval_tu\":" << beacon.interval_tu;
                        if(beacon.advertised_ds_channel)
                            std::cout << ",\"advertised_ds_channel\":" << *beacon.advertised_ds_channel;
                    }catch(const std::invalid_argument&){
                        std::cout << ",\"management_kind\":null";
                    }
                }
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
