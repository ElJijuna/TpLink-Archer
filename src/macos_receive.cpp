#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "archer/firmware_channel.hpp"
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
// One read per bulk IN pipe. Reads consume queued data. No OUT transfer is sent.
static int receive(io_service_t service, const std::filesystem::path& directory) {
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
    try {
        if (!std::filesystem::create_directory(directory))
            throw std::runtime_error("Output directory must be new");
        std::ofstream manifest(directory / "receive.jsonl");
        if (!manifest) throw std::runtime_error("Cannot create receive manifest");
        UInt8 endpoints = 0;
        status = (*usb)->GetNumEndpoints(usb, &endpoints);
        if (status != kIOReturnSuccess) throw std::runtime_error("Cannot enumerate USB pipes");
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
                << ",\"pipe_reference\":" << pipe << ",\"status\":" << status
                << ",\"timeout\":" << (timeout ? "true" : "false")
                << ",\"bytes\":" << (valid ? received : 0) << "}\n";
            std::cout << "endpoint=0x" << std::hex << unsigned(endpoint | 0x80) << std::dec
                      << " status=" << status << " bytes=" << (valid ? received : 0) << '\n';
            if (valid) {
                bytes.resize(received);
                std::ofstream capture(directory / ("endpoint-" + std::to_string(endpoint | 0x80) + ".bin"),
                                      std::ios::binary);
                capture.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                if (!capture) throw std::runtime_error("Cannot save RX bytes");
                try {
                    const auto packets = archer::decode_usb_rx_be(bytes);
                    for (const auto& packet : packets)
                        std::cout << "rx_type=" << packet.packet_type
                                  << " payload_bytes=" << packet.payload_bytes
                                  << " c2h=" << (packet.c2h.has_value() ? "yes" : "no") << '\n';
                } catch (const std::invalid_argument& error) {
                    std::cerr << "Raw bytes retained; BE RX decode failed: " << error.what() << '\n';
                }
                if (result != 2) result = 0;
            } else if (!timeout && status != kIOReturnSuccess) result = 2;
        }
        manifest.flush();
        if (!manifest || inputs != 2) throw std::runtime_error("Incomplete receive inventory");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; result = 2;
    }
    status = (*usb)->USBInterfaceClose(usb);
    (*usb)->Release(usb);
    if (status != kIOReturnSuccess) { std::cerr << "USBInterfaceClose failed\n"; return 2; }
    return result;
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: archer-receive [--with-io-window] NEW_DIRECTORY\n"; return 0;
    }
    const bool window = argc == 3 && std::string(argv[1]) == "--with-io-window";
    if (argc != 2 && !window) { std::cerr << "Usage: archer-receive [--with-io-window] NEW_DIRECTORY\n"; return 2; }
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
            MacRegisterBus bus;
            (void)archer::query_phy_capabilities_with_io_window(bus, [&] {
                result = receive(selected, argv[2]);
            });
        } else result = receive(selected, argv[1]);
    } catch (const std::exception& error) {
        std::cerr << "receive window: " << error.what() << '\n';
        result = 2;
    }
    IOObjectRelease(selected);
    return result;
}
