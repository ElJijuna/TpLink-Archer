#include "archer/ethernet_bridge.hpp"
#include <NetworkingDriverKit/IOUserNetworkEthernet.h>
#include <USBDriverKit/IOUSBHostInterface.h>
// Compile-only ABI boundary check. No service, matching or installation.
archer::BridgeResult archer_bridge_compile_probe(const archer::BridgeMac& station,
    const std::uint8_t* frame,std::size_t size,std::uint8_t* output,std::size_t capacity) {
    return archer::EthernetBridge(station).from_os(frame,size,output,capacity);
}
