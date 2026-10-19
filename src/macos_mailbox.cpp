#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include "archer/register_mailbox.hpp"
#include "archer/usb_request.hpp"

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
class MacRegisterBus final : public archer::RegisterBus {
    IOUSBDeviceInterface197** usb_ = nullptr;
    std::uint32_t transfer(std::uint32_t address, unsigned width,
                           archer::Direction direction, std::uint32_t value) {
        if (width != 1 && width != 2 && width != 4)
            throw std::invalid_argument("Register width must be 1, 2 or 4");
        const auto setup = archer::register_request(direction, address, width);
        std::array<std::uint8_t, 4> data{};
        for (unsigned i = 0; i < width; ++i) data[i] = static_cast<std::uint8_t>(value >> (i * 8));
        IOUSBDevRequestTO request{};
        request.bmRequestType = setup[0]; request.bRequest = setup[1];
        request.wValue = static_cast<UInt16>(setup[2] | (setup[3] << 8));
        request.wIndex = static_cast<UInt16>(setup[4] | (setup[5] << 8));
        request.wLength = width; request.pData = data.data();
        request.noDataTimeout = 100; request.completionTimeout = 200;
        const auto status = (*usb_)->DeviceRequestTO(usb_, &request);
        std::uint32_t observed = 0;
        for (unsigned i = 0; i < width; ++i) observed |= std::uint32_t(data[i]) << (8 * i);
        std::cout << "{\"operation\":\"" << (direction == archer::Direction::read ? "read" : "write")
                  << "\",\"address\":" << address << ",\"width\":" << width
                  << ",\"value\":" << observed << ",\"status\":" << status
                  << ",\"completed_bytes\":" << request.wLenDone << "}" << std::endl;
        if (status != kIOReturnSuccess || request.wLenDone != width)
            throw std::runtime_error("USB register transfer failed or was short");
        return observed;
    }
public:
    MacRegisterBus() {
        io_iterator_t iterator = IO_OBJECT_NULL;
        auto status = IOServiceGetMatchingServices(kIOMainPortDefault,
            IOServiceMatching("IOUSBHostDevice"), &iterator);
        if (status != kIOReturnSuccess) throw std::runtime_error("Cannot enumerate USB devices");
        io_service_t selected = IO_OBJECT_NULL;
        unsigned count = 0;
        while (auto device = IOIteratorNext(iterator)) {
            if (number(device, CFSTR("idVendor")) == 0x37ad && number(device, CFSTR("idProduct")) == 0x0104) {
                ++count;
                if (!selected) { selected = device; continue; }
            }
            IOObjectRelease(device);
        }
        IOObjectRelease(iterator);
        if (count != 1) {
            if (selected) IOObjectRelease(selected);
            throw std::runtime_error("Expected exactly one USB 37ad:0104 device");
        }
        IOCFPlugInInterface** plugin = nullptr;
        SInt32 score = 0;
        status = IOCreatePlugInInterfaceForService(selected, kIOUSBDeviceUserClientTypeID,
            kIOCFPlugInInterfaceID, &plugin, &score);
        IOObjectRelease(selected);
        if (status != kIOReturnSuccess || !plugin)
            throw std::runtime_error("USB client unavailable; check host access permissions");
        auto queried = (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197),
                                                reinterpret_cast<LPVOID*>(&usb_));
        IODestroyPlugInInterface(plugin);
        if (queried || !usb_) throw std::runtime_error("USB interface unavailable");
    }
    ~MacRegisterBus() override { if (usb_) (*usb_)->Release(usb_); }
    MacRegisterBus(const MacRegisterBus&) = delete;
    MacRegisterBus& operator=(const MacRegisterBus&) = delete;
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        return transfer(address, width, archer::Direction::read, 0);
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        (void)transfer(address, width, archer::Direction::write, value);
    }
    void pause() override { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
};
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
