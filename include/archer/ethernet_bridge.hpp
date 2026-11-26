#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace archer {
using BridgeMac = std::array<std::uint8_t, 6>;
enum class BridgeStatus { ok, invalid_station_or_mtu, invalid_extent, wrong_source,
    wrong_receiver_or_reflection, unsupported_protocol, unsupported_snap,
    short_output, overlapping_buffers };
struct BridgeResult {
    BridgeStatus status;
    std::size_t bytes = 0;
    BridgeMac destination{};
    explicit operator bool() const { return status==BridgeStatus::ok; }
};

// Ethernet II without FCS/VLAN -> RFC1042 SNAP, IPv4/ARP/IPv6, MTU <=1500.
// No allocation, exceptions, keys, USB access or link-state transitions.
class EthernetBridge {
    BridgeMac station_;
    std::size_t mtu_;
    bool valid_;
    static bool unicast(const BridgeMac& mac) {
        return !(mac[0]&1) && std::any_of(mac.begin(),mac.end(),[](auto b){return b!=0;});
    }
    static bool protocol(const std::uint8_t* type) {
        const unsigned value=(unsigned(type[0])<<8)|type[1];
        return value==0x0800 || value==0x0806 || value==0x86dd;
    }
    static BridgeMac mac_at(const std::uint8_t* bytes) {
        BridgeMac mac{};std::copy_n(bytes,6,mac.begin());return mac;
    }
    static bool overlaps(const void* a,std::size_t na,const void* b,std::size_t nb) {
        const auto x=reinterpret_cast<std::uintptr_t>(a),y=reinterpret_cast<std::uintptr_t>(b);
        return x<=y ? y-x<na : x-y<nb;
    }
public:
    explicit EthernetBridge(BridgeMac station,std::size_t mtu=1500)
        :station_(station),mtu_(mtu),valid_(unicast(station)&&mtu&&mtu<=1500) {}

    BridgeResult from_os(const std::uint8_t* frame,std::size_t size,
                         std::uint8_t* output,std::size_t capacity) const {
        if(!valid_)return {BridgeStatus::invalid_station_or_mtu};
        if(!frame||size<15||size>14+mtu_)return {BridgeStatus::invalid_extent};
        if(mac_at(frame+6)!=station_)return {BridgeStatus::wrong_source};
        const auto destination=mac_at(frame);
        if(std::all_of(destination.begin(),destination.end(),[](auto b){return b==0;}))
            return {BridgeStatus::wrong_receiver_or_reflection};
        // EAPOL belongs to the supplicant. VLAN, 802.3 length and unknown types
        // require separate implementation; do not advertise them to the OS.
        if(!protocol(frame+12))return {BridgeStatus::unsupported_protocol};
        const auto length=size-6;
        if(!output||capacity<length)return {BridgeStatus::short_output};
        if(overlaps(frame,size,output,length))return {BridgeStatus::overlapping_buffers};
        const std::array<std::uint8_t,6> snap{0xaa,0xaa,0x03,0,0,0};
        std::copy(snap.begin(),snap.end(),output);
        std::copy(frame+12,frame+size,output+6);
        return {BridgeStatus::ok,length,destination};
    }
    // Call ONLY after CCMP MIC, PN, BSSID/direction and receiver checks succeed.
    // destination/source are FromDS addr1/addr3 from that authenticated frame.
    BridgeResult from_authenticated_ap(const std::uint8_t* msdu,std::size_t size,
                         BridgeMac destination,BridgeMac source,
                         std::uint8_t* output,std::size_t capacity) const {
        if(!valid_)return {BridgeStatus::invalid_station_or_mtu};
        if(!msdu||size<9||size>8+mtu_)return {BridgeStatus::invalid_extent};
        const std::array<std::uint8_t,6> snap{0xaa,0xaa,0x03,0,0,0};
        if(!std::equal(snap.begin(),snap.end(),msdu))return {BridgeStatus::unsupported_snap};
        if((!(destination[0]&1)&&destination!=station_)||!unicast(source)||source==station_)
            return {BridgeStatus::wrong_receiver_or_reflection};
        if(!protocol(msdu+6))return {BridgeStatus::unsupported_protocol};
        const auto length=size+6;
        if(!output||capacity<length)return {BridgeStatus::short_output};
        if(overlaps(msdu,size,output,length))return {BridgeStatus::overlapping_buffers};
        std::copy(destination.begin(),destination.end(),output);
        std::copy(source.begin(),source.end(),output+6);
        std::copy(msdu+6,msdu+size,output+12);
        return {BridgeStatus::ok,length,destination};
    }
};
}
