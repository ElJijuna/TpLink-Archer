#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <iostream>

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
int main() {
    io_iterator_t iterator = IO_OBJECT_NULL;
    auto status = IOServiceGetMatchingServices(kIOMainPortDefault,
        IOServiceMatching("IOUSBHostDevice"), &iterator);
    if (status != KERN_SUCCESS) {
        std::cerr << "IORegistry enumeration failed: " << status << '\n';
        return 2;
    }
    unsigned count = 0;
    while (auto device = IOIteratorNext(iterator)) {
        if (number(device, CFSTR("idVendor")) == 0x37ad &&
            number(device, CFSTR("idProduct")) == 0x0104) {
            ++count;
            std::cout << "{\"vendor_id\":\"37ad\",\"product_id\":\"0104\","
                      << "\"location_id\":" << number(device, CFSTR("locationID"))
                      << ",\"implementation_status\":\"diagnostic_only\",\"source\":\"IORegistry\"}\n";
        }
        IOObjectRelease(device);
    }
    IOObjectRelease(iterator);
    if (!count) std::cerr << "Target USB 37ad:0104 not detected\n";
    return count ? 0 : 1;
}
