#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include "archer/firmware_channel.hpp"
#include "archer/command_tracker.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/macos_register_bus.hpp"

static long number(io_registry_entry_t device, CFStringRef key) {
    auto value = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
    long result = -1;
    if (value) {
        if (CFGetTypeID(value) == CFNumberGetTypeID())
            CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberLongType, &result);
        CFRelease(value);
    }
    return result;
}
// Passive mode reads once per pipe; diagnostic mode sends once and reads bounded rounds.
static int receive(io_service_t service, const std::filesystem::path& directory, bool diagnostic = false, unsigned sequence = 0) {
    IOCFPlugInInterface** plugin = nullptr;
    SInt32 score = 0;
    auto status = IOCreatePlugInInterfaceForService(service, kIOUSBInterfaceUserClientTypeID,
        kIOCFPlugInInterfaceID, &plugin, &score);
    if (status != kIOReturnSuccess || !plugin) {
        std::cerr << "USB interface client unavailable: " << status << '\n'; return 2;
    }
    IOUSBInterfaceInterface300** usb = nullptr;
    auto queried = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
                                            reinterpret_cast<LPVOID*>(&usb));
    IODestroyPlugInInterface(plugin);
    if (queried || !usb) { std::cerr << "QueryInterface failed\n"; return 2; }
    status = (*usb)->USBInterfaceOpen(usb);
    if (status != kIOReturnSuccess) {
        (*usb)->Release(usb);
        std::cerr << "USBInterfaceOpen failed: " << status << '\n'; return 2;
    }
    int result = 3; // No data during this bounded attempt.
    unsigned inputs = 0;
    archer::CommandTracker tracker(1, 0, 0, sequence);
    try {
        if (!std::filesystem::create_directory(directory))
            throw std::runtime_error("Output directory must be new");
        std::ofstream manifest(directory / "receive.jsonl");
        if (!manifest) throw std::runtime_error("Cannot create receive manifest");
        UInt8 endpoints = 0;
        status = (*usb)->GetNumEndpoints(usb, &endpoints);
        if (status != kIOReturnSuccess) throw std::runtime_error("Cannot enumerate USB pipes");
        if (diagnostic) {
            if (endpoints != 9) throw std::runtime_error("Unexpected endpoint inventory");
            UInt8 output = 0;
            for (unsigned pipe = 1; pipe <= endpoints; ++pipe) {
                UInt8 direction = 0, endpoint = 0, type = 0, interval = 0;
                UInt16 packet_size = 0;
                status = (*usb)->GetPipeProperties(usb, pipe, &direction, &endpoint,
                                                  &type, &packet_size, &interval);
                if (status != kIOReturnSuccess) throw std::runtime_error("Cannot inspect TX pipe");
                if (direction == kUSBOut && endpoint == 7 && type == kUSBBulk && packet_size == 512) {
                    if (output) throw std::runtime_error("Ambiguous H2C endpoint");
                    output = static_cast<UInt8>(pipe);
                }
            }
            if (!output) throw std::runtime_error("Expected USB2 H2C endpoint 0x07 absent");
            auto command = archer::diagnostic_log_disable(sequence);
            std::ofstream tx(directory / "h2c-log-disable.bin", std::ios::binary);
            tx.write(reinterpret_cast<const char*>(command.data()), command.size());
            tx.close();
            if (!tx) throw std::runtime_error("Cannot preserve outgoing command");
            status = (*usb)->WritePipeTO(usb, output, command.data(), command.size(), 500, 1000);
            manifest << "{\"operation\":\"bulk_write\",\"endpoint_address\":7,\"pipe_reference\":"
                     << unsigned(output) << ",\"requested_bytes\":" << command.size()
                     << ",\"status\":" << status << "}\n";
            manifest.flush();
            if (status != kIOReturnSuccess) throw std::runtime_error("H2C write failed; no retry");
        }
        for (unsigned round = 0; round < (diagnostic ? 3u : 1u); ++round) {
        inputs = 0;
        for (unsigned pipe = 1; pipe <= endpoints; ++pipe) {
            UInt8 direction = 0, endpoint = 0, type = 0, interval = 0;
            UInt16 packet_size = 0;
            status = (*usb)->GetPipeProperties(usb, pipe, &direction, &endpoint, &type,
                                              &packet_size, &interval);
            if (status != kIOReturnSuccess) throw std::runtime_error("Cannot inspect USB pipe");
            if (direction != kUSBIn || type != kUSBBulk) continue;
            if (++inputs > 2) throw std::runtime_error("Unexpected number of input pipes");
            std::vector<std::uint8_t> bytes(20480);
            UInt32 received = static_cast<UInt32>(bytes.size());
            status = (*usb)->ReadPipeTO(usb, pipe, bytes.data(), &received, 500, 1000);
            if (received > bytes.size()) throw std::runtime_error("Invalid USB read length");
            const bool valid = status == kIOReturnSuccess && received > 0;
            const bool timeout = status == kIOUSBTransactionTimeout || status == kIOReturnTimeout;
            auto& out = manifest;
            out << "{\"endpoint_address\":" << unsigned(endpoint | 0x80)
                << ",\"round\":" << round << ",\"pipe_reference\":" << pipe << ",\"status\":" << status
                << ",\"timeout\":" << (timeout ? "true" : "false")
                << ",\"bytes\":" << (valid ? received : 0) << "}\n";
            std::cout << "endpoint=0x" << std::hex << unsigned(endpoint | 0x80) << std::dec
                      << " status=" << status << " bytes=" << (valid ? received : 0) << '\n';
            if (valid) {
                bytes.resize(received);
                std::ofstream capture(directory / ("endpoint-" + std::to_string(endpoint | 0x80) + "-round-" + std::to_string(round) + ".bin"),
                                      std::ios::binary);
                capture.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                if (!capture) throw std::runtime_error("Cannot save RX bytes");
                try {
                    const auto packets = archer::decode_usb_rx_be(bytes);
                    for (const auto& packet : packets) {
                        if (diagnostic && packet.c2h) {
                            const bool matched = tracker.observe(bytes.data() + packet.payload_offset, packet.payload_bytes);
                            std::cout << "ack_matches_command=" << (matched ? "yes" : "no") << '\n';
                        }
                        std::cout << "rx_type=" << packet.packet_type
                                  << " payload_bytes=" << packet.payload_bytes
                                  << " c2h=" << (packet.c2h.has_value() ? "yes" : "no") << '\n';
                    }
                } catch (const std::invalid_argument& error) {
                    std::cerr << "Raw bytes retained; BE RX decode failed: " << error.what() << '\n';
                    if (diagnostic) throw std::runtime_error("Invalid diagnostic event");
                }
                if (result != 2) result = 0;
            } else if (!timeout && status != kIOReturnSuccess) result = 2;
        }
        if (inputs != 2) throw std::runtime_error("Incomplete receive inventory");
        if (diagnostic && tracker.received && tracker.completion) break;
        }
        if (diagnostic) {
            manifest << "{\"result\":\"command_tracking\",\"sequence\":" << sequence
                     << ",\"received\":" << (tracker.received ? "true" : "false")
                     << ",\"completion_code\":";
            if (tracker.completion) manifest << *tracker.completion;
            else manifest << "null";
            manifest << "}\n";
        }
        manifest.flush();
        if (!manifest || inputs != 2) throw std::runtime_error("Incomplete receive inventory");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; result = 2;
    }
    status = (*usb)->USBInterfaceClose(usb);
    (*usb)->Release(usb);
    if (status != kIOReturnSuccess) { std::cerr << "USBInterfaceClose failed\n"; return 2; }
    if (diagnostic && result != 2) {
        if (tracker.completion) return *tracker.completion == 0 ? 0 : 5;
        return tracker.received ? 6 : 4;
    }
    return result;
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: archer-receive [--with-io-window | --diagnostic-log-disable] NEW_DIRECTORY\n"; return 0;
    }
    const bool diagnostic = argc == 3 && std::string(argv[1]) == "--diagnostic-log-disable";
    const bool window = diagnostic || (argc == 3 && std::string(argv[1]) == "--with-io-window");
    if (argc != 2 && !window) { std::cerr << "Usage: archer-receive [--with-io-window | --diagnostic-log-disable] NEW_DIRECTORY\n"; return 2; }
    io_iterator_t iterator = IO_OBJECT_NULL;
    auto status = IOServiceGetMatchingServices(kIOMainPortDefault,
        IOServiceMatching("IOUSBHostInterface"), &iterator);
    if (status != kIOReturnSuccess) return 2;
    io_service_t selected = IO_OBJECT_NULL;
    unsigned matches = 0;
    while (auto interface = IOIteratorNext(iterator)) {
        if (number(interface, CFSTR("idVendor")) == 0x37ad &&
            number(interface, CFSTR("idProduct")) == 0x0104 &&
            number(interface, CFSTR("bInterfaceNumber")) == 0) {
            ++matches;
            if (!selected) { selected = interface; continue; }
        }
        IOObjectRelease(interface);
    }
    IOObjectRelease(iterator);
    if (matches != 1) {
        if (selected) IOObjectRelease(selected);
        std::cerr << "Expected exactly one Archer interface; found " << matches << '\n';
        return matches ? 2 : 1;
    }
    int result = 2;
    try {
        if (window) {
            std::unique_ptr<archer::SequenceLedger> ledger;
            if (diagnostic) ledger = std::make_unique<archer::SequenceLedger>("artifacts/37ad-0104-sequences.bin");
            MacRegisterBus bus;
            (void)archer::query_phy_capabilities_with_io_window(bus, [&] {
                if (diagnostic) {
                    for (auto address : {0x8d00u, 0x9100u})
                        if ((bus.read(address, 4) & 3) != 3)
                            throw std::runtime_error("DLE not ready; no bulk command sent");
                    if (!(bus.read(0x7880, 4) & 1))
                        throw std::runtime_error("HCI TX DMA disabled");
                }
                if (diagnostic) {
                    // Keep this ledger for the adapter; do not delete it between runs.
                    for (unsigned command = 0; command < 2; ++command) {
                        const auto sequence = ledger->reserve();
                        result = receive(selected, std::filesystem::path(argv[2]).string() + "-command-" + std::to_string(command), true, sequence);
                        if (result != 0) break;
                    }
                } else result = receive(selected, argv[2]);
            });
        } else result = receive(selected, argv[1]);
    } catch (const std::exception& error) {
        std::cerr << "receive window: " << error.what() << '\n';
        result = 2;
    }
    IOObjectRelease(selected);
    return result;
}
