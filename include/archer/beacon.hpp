#pragma once
#include "archer/rx_descriptor.hpp"
#include <array>
namespace archer {
struct BeaconSummary {
    bool probe_response=false;
    unsigned interval_tu=0, capability=0;
    std::array<std::uint8_t,6> bssid{};
    std::vector<std::uint8_t> ssid; // bytes, not assumed UTF-8
    std::optional<unsigned> advertised_ds_channel;
};
// Input is an unconverted MPDU with FCS already removed by an evidenced policy.
// This strict subset rejects fragmented/protected/HT-control management frames.
inline BeaconSummary parse_beacon(const std::uint8_t* p,std::size_t n) {
    if(!p || n<36)throw std::invalid_argument("Truncated beacon fixed fields");
    const unsigned fc=p[0]|(unsigned(p[1])<<8);
    if((fc&0xf)!=0 || ((fc&0xf0)!=0x80 && (fc&0xf0)!=0x50) ||
       (fc&0xc700) || (p[22]&15))
        throw std::invalid_argument("Unsupported beacon framing");
    BeaconSummary r;r.probe_response=(fc&0xf0)==0x50;
    std::copy(p+16,p+22,r.bssid.begin());
    r.interval_tu=p[32]|(unsigned(p[33])<<8);r.capability=p[34]|(unsigned(p[35])<<8);
    bool ssid_seen=false;
    for(std::size_t at=36;at<n;) {
        if(n-at<2)throw std::invalid_argument("Truncated information element header");
        const unsigned id=p[at++],len=p[at++];
        if(len>n-at)throw std::invalid_argument("Truncated information element body");
        if(id==0){
            if(ssid_seen || len>32)throw std::invalid_argument("Invalid or duplicate SSID element");
            ssid_seen=true;r.ssid.assign(p+at,p+at+len);
        }else if(id==3){
            if(r.advertised_ds_channel || len!=1 || p[at]<1 || p[at]>14)
                throw std::invalid_argument("Invalid or duplicate DS channel element");
            r.advertised_ds_channel=p[at];
        }
        at+=len;
    }
    if(!ssid_seen)throw std::invalid_argument("Missing SSID element");
    return r;
}
inline BeaconSummary decode_rx_beacon(const std::vector<std::uint8_t>& bytes,
    const BeRxMetadata& m) {
    // Revalidate offsets: metadata can be caller-constructed.
    const auto at=m.packet.payload_offset,n=m.packet.payload_bytes;
    if(at>bytes.size() || n>bytes.size()-at || !m.normal_80211_candidate())
        throw std::invalid_argument("Not a bounded unconverted error-free RX frame");
    return parse_beacon(bytes.data()+at,m.payload_without_fcs_bytes());
}
}
