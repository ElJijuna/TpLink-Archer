#pragma once
#include "archer/passive_scan.hpp"
#include <set>
namespace archer {
// Audit the recorded channel1 experiment; no TX, RSSI or air-emission inference.
class ProbeAirAudit {
    unsigned responses_=0,bad_fcs_=0,accepted_=0;
    std::set<std::array<std::uint8_t,6>> responding_,channel1_,missing_basic_;
public:
    explicit ProbeAirAudit(unsigned channel){if(channel!=1)throw std::invalid_argument("Probe experiment receive channel must be1");}
    bool observe(const std::vector<std::uint8_t>& bytes,const BeRxMetadata& m){
        auto at=m.packet.payload_offset,n=m.packet.payload_bytes;
        if(!m.normal_80211_candidate()||m.fcs_policy!=RxFcsPolicy::appended||at>bytes.size()||n>bytes.size()-at||n<4)return false;
        const auto p=bytes.data()+at;
        if(ieee_fcs(p,n-4)!=channel_u32(p+n-4)){++bad_fcs_;return false;}
        BeaconSummary b;
        try{b=decode_rx_beacon(bytes,m);}catch(const std::invalid_argument&){return false;}
        bool nonzero=false;for(auto v:b.bssid)nonzero|=v!=0;
        if(!nonzero||(b.bssid[0]&1))return false;
        ++accepted_;
        const std::array<std::uint8_t,6> station{2,0,0,0x89,0x22,0xa1};
        if(b.probe_response&&std::equal(station.begin(),station.end(),p+4)){
            ++responses_;responding_.insert(b.bssid);
        }
        if(!b.probe_response&&b.advertised_ds_channel==1){
            channel1_.insert(b.bssid);
            for(std::size_t pos=36;pos<n-4;){
                const auto id=p[pos++],len=p[pos++]; // Bounds already validated by parse_beacon.
                if(id==1||id==50)for(unsigned i=0;i<len;++i){
                    const auto rate=p[pos+i];
                    if((rate&128)&&((rate&127)!=2&&(rate&127)!=4&&(rate&127)!=11&&(rate&127)!=22))
                        missing_basic_.insert(b.bssid);
                }
                pos+=len;
            }
        }
        return true;
    }
    unsigned responses()const{return responses_;}
    unsigned missing_basic()const{return missing_basic_.size();}
    unsigned bad_fcs()const{return bad_fcs_;}
    void write_json(std::ostream& out)const{
        out<<"{\"scope\":\"recorded_channel1_probe_experiment\",\"accepted_management\":"<<accepted_
           <<",\"bad_fcs\":"<<bad_fcs_<<",\"probe_responses_to_test_mac\":"<<responses_
           <<",\"responding_bssids\":"<<responding_.size()<<",\"channel1_beacon_bssids\":"<<channel1_.size()
           <<",\"bssids_with_basic_rates_absent_from_cck_only_probe\":"<<missing_basic_.size()
           <<",\"absence_proves_tx_failure\":false}\n";
    }
};
}
