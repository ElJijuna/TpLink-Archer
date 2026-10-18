#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <iostream>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include "archer/usb_descriptors.hpp"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "archer/usb_request.hpp"
#include <string>

static long number(io_registry_entry_t device, CFStringRef key) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
    long result = -1;
    if (value) {
        if (CFGetTypeID(value) == CFNumberGetTypeID())
            CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberLongType, &result);
        CFRelease(value);
    }
    return result;
}
// Descriptor access requires neither USBDeviceOpen nor exclusive ownership.
static bool describe(io_service_t service, const std::string& dump, bool read_chip, bool startup) {
    IOCFPlugInInterface** plugin = nullptr;
    SInt32 score = 0;
    auto status = IOCreatePlugInInterfaceForService(service, kIOUSBDeviceUserClientTypeID,
                                                    kIOCFPlugInInterfaceID, &plugin, &score);
    if (status != kIOReturnSuccess || !plugin) {
        std::cerr << "Cannot create USB descriptor client: " << status << '\n';
        return false;
    }
    IOUSBDeviceInterface197** usb = nullptr;
    auto queried = (*plugin)->QueryInterface(plugin,
        CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197), reinterpret_cast<LPVOID*>(&usb));
    IODestroyPlugInInterface(plugin);
    if (queried || !usb) {
        std::cerr << "USB interface unavailable: " << queried << '\n';
        return false;
    }
    if (read_chip) {
        const std::vector<std::uint32_t> registers = startup
            ? std::vector<std::uint32_t>{0xf0, 0xfc, 0x1e0}
            : std::vector<std::uint32_t>{0xfc};
        bool success = true;
        for (auto address : registers) {
            // Fixed read-only profile from Linux reg.h; no writes or power changes.
            const auto setup = archer::register_request(archer::Direction::read, address, 4);
            std::array<std::uint8_t, 4> data{};
            IOUSBDevRequestTO request{};
            request.bmRequestType = setup[0]; request.bRequest = setup[1];
            request.wValue = archer::little16(setup.data() + 2);
            request.wIndex = archer::little16(setup.data() + 4);
            request.wLength = archer::little16(setup.data() + 6);
            request.pData = data.data();
            request.noDataTimeout = 500; request.completionTimeout = 1000;
            status = (*usb)->DeviceRequestTO(usb, &request);
            if (status != kIOReturnSuccess || request.wLenDone != data.size()) {
                std::cerr << "Register read failed: address=" << address << " status=" << status
                          << " bytes=" << request.wLenDone << '\n';
                success = false;
                break;
            }
            const std::uint32_t value = std::uint32_t(data[0]) | (std::uint32_t(data[1]) << 8) |
                (std::uint32_t(data[2]) << 16) | (std::uint32_t(data[3]) << 24);
            std::cout << "{\"register\":\"0x" << std::hex << address << std::dec
                      << "\",\"raw_value\":" << value;
            if (address == 0xfc) std::cout << ",\"hardware_id_low_byte\":" << unsigned(data[0]);
            if (address == 0xf0) std::cout << ",\"chip_version_bits_15_12\":" << ((value >> 12) & 15);
            if (address == 0x1e0) std::cout << ",\"firmware_status_bits_29_26\":" << ((value >> 26) & 15);
            std::cout << ",\"interpretation\":\"raw_observation_only\"}\n";
        }
        (*usb)->Release(usb);
        return success;
    }
    IOUSBConfigurationDescriptorPtr descriptor = nullptr;
    status = (*usb)->GetConfigurationDescriptorPtr(usb, 0, &descriptor);
    bool success = false;
    if (status == kIOReturnSuccess && descriptor) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(descriptor);
        auto size = archer::little16(bytes + 2);
        try {
            auto config = archer::parse_configuration(bytes, size);
            if (!dump.empty()) {
                const int fd = open(dump.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
                if (fd < 0) throw std::runtime_error("Cannot create new descriptor file");
                const auto written = write(fd, bytes, size);
                const auto closed = close(fd);
                if (written != size || closed != 0)
                    throw std::runtime_error("Incomplete descriptor file; discard it");
            }
            for (const auto& endpoint : config.endpoints) {
                std::cout << "{\"configuration\":" << unsigned(config.value)
                          << ",\"interface\":" << unsigned(endpoint.interface_number)
                          << ",\"alternate_setting\":" << unsigned(endpoint.alternate_setting)
                          << ",\"endpoint_address\":" << unsigned(endpoint.address)
                          << ",\"direction\":\"" << (endpoint.input() ? "in" : "out")
                          << "\",\"transfer_type\":" << unsigned(endpoint.attributes & 3)
                          << ",\"max_packet_size\":" << endpoint.max_packet_size
                          << ",\"interval\":" << unsigned(endpoint.interval) << "}\n";
            }
            success = true;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
        }
    } else std::cerr << "Cannot read configuration descriptor: " << status << '\n';
    (*usb)->Release(usb);
    return success;
}
int main(int argc, char** argv) {
    bool descriptors = false;
    bool read_chip = false;
    bool startup = false;
    std::string dump;
    if (argc == 2 && std::string(argv[1]) == "--descriptors") descriptors = true;
    else if (argc == 2 && std::string(argv[1]) == "--read-chip-info") {
        descriptors = true; read_chip = true;
    }
    else if (argc == 2 && std::string(argv[1]) == "--read-startup-state") {
        descriptors = true; read_chip = true; startup = true;
    }
    else if (argc == 3 && std::string(argv[1]) == "--dump-descriptors") {
        descriptors = true; dump = argv[2];
    } else if (argc != 1) {
        std::cerr << "Usage: archer-probe [--descriptors | --dump-descriptors NEW_FILE | --read-chip-info | --read-startup-state]\n";
        return 2;
    }
    io_iterator_t iterator = IO_OBJECT_NULL;
    auto status = IOServiceGetMatchingServices(kIOMainPortDefault,
        IOServiceMatching("IOUSBHostDevice"), &iterator);
    if (status != KERN_SUCCESS) {
        std::cerr << "IORegistry enumeration failed: " << status << '\n';
        return 2;
    }
    unsigned count = 0;
    bool success = true;
    while (auto device = IOIteratorNext(iterator)) {
        if (number(device, CFSTR("idVendor")) == 0x37ad &&
            number(device, CFSTR("idProduct")) == 0x0104) {
            ++count;
            std::cout << "{\"vendor_id\":\"37ad\",\"product_id\":\"0104\","
                      << "\"location_id\":" << number(device, CFSTR("locationID"))
                      << ",\"implementation_status\":\"diagnostic_only\",\"source\":\"IORegistry\"}\n";
            if (descriptors && !describe(device, dump, read_chip, startup)) success = false;
        }
        IOObjectRelease(device);
    }
    IOObjectRelease(iterator);
    if (!count) std::cerr << "Target USB 37ad:0104 not detected\n";
    return !count ? 1 : success ? 0 : 2;
}
