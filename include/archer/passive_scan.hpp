#pragma once
#include "archer/beacon.hpp"
#include <map>
#include <tuple>
#include <ostream>
namespace archer {
inline std::uint32_t ieee_fcs(const std::uint8_t* p,std::size_t n){
    std::uint32_t crc=0xffffffff;
    for(std::size_t i=0;i<n;++i){crc^=p[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}
    return ~crc;
}
struct PassiveNetwork {
    BeaconSummary beacon;unsigned beacons=0,probe_responses=0;
    unsigned first_observation=0,last_observation=0;
};
// Explicit per-capture receive channel. This aggregator never retunes or sends TX.
// Keep conflicting SSID/channel advertisements separate rather than guessing
// identity or merging hidden and disclosed SSIDs. No RF channel/RSSI inference.
class PassiveScan {
    using Key=std::tuple<std::array<std::uint8_t,6>,std::vector<std::uint8_t>,std::optional<unsigned>>;
    std::map<Key,PassiveNetwork> networks_;
    unsigned receive_channel_=1;
    unsigned normal_=0,accepted_=0,bad_fcs_=0,unsupported_=0,overflow_=0;
public:
    explicit PassiveScan(unsigned channel=1):receive_channel_(channel){
        if(channel!=1&&channel!=6&&channel!=11)throw std::invalid_argument("Passive capture channel must be 1/6/11");
    }
    bool observe(const std::vector<std::uint8_t>& bytes,const BeRxMetadata& m){
        if(m.packet.packet_type!=0)return false;
        ++normal_;
        const auto at=m.packet.payload_offset,n=m.packet.payload_bytes;
        if(!m.normal_80211_candidate()||m.fcs_policy!=RxFcsPolicy::appended||at>bytes.size()||n>bytes.size()-at||n<4){++unsupported_;return false;}
        const auto* p=bytes.data()+at;
        if(ieee_fcs(p,n-4)!=channel_u32(p+n-4)){++bad_fcs_;return false;}
        BeaconSummary b;
        try{b=decode_rx_beacon(bytes,m);}catch(const std::invalid_argument&){++unsupported_;return false;}
        // BSSID must identify an individual transmitter; do not invent one.
        bool nonzero=false;for(auto v:b.bssid)nonzero|=v!=0;
        if(!nonzero||(b.bssid[0]&1)){++unsupported_;return false;}
        const Key key{b.bssid,b.ssid,b.advertised_ds_channel};
        auto it=networks_.find(key);
        if(it==networks_.end()){
            if(networks_.size()>=128){++overflow_;return false;}
            it=networks_.emplace(key,PassiveNetwork{b,0,0,normal_,normal_}).first;
        }
        auto& e=it->second;e.beacon=b;e.last_observation=normal_;
        if(b.probe_response)++e.probe_responses;else ++e.beacons;
        ++accepted_;return true;
    }
    const auto& networks()const{return networks_;}
    unsigned accepted()const{return accepted_;}
    unsigned bad_fcs()const{return bad_fcs_;}
    unsigned overflow()const{return overflow_;}
    void write_json(std::ostream& out)const{
        auto hex=[&](const auto& bytes){const char* h="0123456789abcdef";for(auto b:bytes)out<<h[b>>4]<<h[b&15];};
        out<<"{\"receive_channel\":"<<receive_channel_<<",\"scope\":\"all_receive_phases\",\"active_probes_sent\":0,\"normal_packets\":"<<normal_
           <<",\"accepted_management\":"<<accepted_<<",\"bad_fcs\":"<<bad_fcs_<<",\"unsupported\":"<<unsupported_
           <<",\"table_overflow\":"<<overflow_<<",\"networks\":[";
        bool comma=false;
        for(const auto& pair:networks_){const auto& e=pair.second;const auto& b=e.beacon;
            if(comma)out<<',';comma=true;
            out<<"{\"bssid_hex\":\"";hex(b.bssid);out<<"\",\"ssid_hex\":\"";hex(b.ssid);
            out<<"\",\"advertised_ds_channel\":";if(b.advertised_ds_channel)out<<*b.advertised_ds_channel;else out<<"null";
            out<<",\"beacons\":"<<e.beacons<<",\"probe_responses\":"<<e.probe_responses
               <<",\"first_observation\":"<<e.first_observation<<",\"last_observation\":"<<e.last_observation
               <<",\"interval_tu\":"<<b.interval_tu<<",\"capability\":"<<b.capability<<"}";
        }
        out<<"]}\n";
    }
};
}
