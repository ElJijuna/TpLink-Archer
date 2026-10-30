#include "archer/probe_air_audit.hpp"
#include <cassert>
#include <iostream>
std::vector<std::uint8_t> packet(bool response,bool addressed,bool ofdm){
 std::vector<std::uint8_t> p(36);p[0]=response?0x50:0x80;p[16]=2;p[21]=1;
 const std::array<std::uint8_t,6> station{2,0,0,0x89,0x22,0xa1};
 if(addressed)std::copy(station.begin(),station.end(),p.begin()+4);
 p.insert(p.end(),{0,1,'x',3,1,1,1,1,static_cast<std::uint8_t>(ofdm?0x8c:0x82)});
 auto fcs=archer::ieee_fcs(p.data(),p.size());for(unsigned i=0;i<4;++i)p.push_back(fcs>>(8*i));return p;
}
archer::BeRxMetadata meta(const std::vector<std::uint8_t>& p){archer::BeRxMetadata m;m.packet.payload_bytes=p.size();m.header_converted=false;m.fcs_policy=archer::RxFcsPolicy::appended;return m;}
int main(){
 archer::ProbeAirAudit audit(1);auto beacon=packet(false,false,true);assert(audit.observe(beacon,meta(beacon)));assert(audit.missing_basic()==1);
 auto other=packet(true,false,false);assert(audit.observe(other,meta(other)));assert(audit.responses()==0);
 auto response=packet(true,true,false);assert(audit.observe(response,meta(response)));assert(audit.responses()==1);
 response.back()^=1;assert(!audit.observe(response,meta(response)));assert(audit.responses()==1&&audit.bad_fcs()==1);
 auto bad=meta(beacon);bad.packet.payload_offset=beacon.size()+1;assert(!audit.observe(beacon,bad));
 bool rejected=false;try{archer::ProbeAirAudit unsupported(6);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
 std::cout<<"Probe response address, software FCS and advertised basic rates: OK\n";
}
