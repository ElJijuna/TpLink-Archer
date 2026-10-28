#include "archer/passive_scan.hpp"
#include <cassert>
#include <sstream>
#include <iostream>
std::vector<std::uint8_t> frame(unsigned id=1,bool probe=false,unsigned channel=1,std::vector<std::uint8_t> ssid={'a',0,'b'}){
    std::vector<std::uint8_t> b(36);b[0]=probe?0x50:0x80;b[16]=2;b[21]=id;b[32]=100;
    b.push_back(0);b.push_back(ssid.size());b.insert(b.end(),ssid.begin(),ssid.end());
    b.insert(b.end(),{3,1,static_cast<std::uint8_t>(channel)});
    const auto fcs=archer::ieee_fcs(b.data(),b.size());
    for(unsigned i=0;i<4;++i)b.push_back(fcs>>(8*i));return b;
}
archer::BeRxMetadata metadata(const std::vector<std::uint8_t>& b){
    archer::BeRxMetadata m;m.packet.payload_bytes=b.size();m.header_converted=false;m.fcs_policy=archer::RxFcsPolicy::appended;return m;
}
int main(){
    const std::string known="123456789";assert(archer::ieee_fcs(reinterpret_cast<const std::uint8_t*>(known.data()),known.size())==0xcbf43926);
    archer::PassiveScan s;auto b=frame();auto m=metadata(b);
    assert(s.observe(b,m));assert(s.observe(b,m));auto p=frame(1,true);assert(s.observe(p,metadata(p)));
    assert(s.networks().size()==1&&s.accepted()==3);
    const auto& e=s.networks().begin()->second;assert(e.beacons==2&&e.probe_responses==1&&e.first_observation==1&&e.last_observation==3);
    auto other=frame(1,false,2);assert(s.observe(other,metadata(other))&&s.networks().size()==2);
    auto hidden=frame(1,false,1,{});assert(s.observe(hidden,metadata(hidden))&&s.networks().size()==3);
    b.back()^=1;assert(!s.observe(b,m)&&s.bad_fcs()==1);b.back()^=1;
    m.packet.payload_offset=b.size()+1;assert(!s.observe(b,m));m=metadata(b);m.crc_error=true;assert(!s.observe(b,m));
    m=metadata(b);m.fcs_policy=archer::RxFcsPolicy::unknown;assert(!s.observe(b,m));
    std::ostringstream out;s.write_json(out);assert(out.str().find("610062")!=std::string::npos&&out.str().find("\"receive_channel\":1")!=std::string::npos);
    archer::PassiveScan scan6(6);auto f6=frame(1,false,1);assert(scan6.observe(f6,metadata(f6)));
    std::ostringstream six;scan6.write_json(six);
    assert(six.str().find("\"receive_channel\":6")!=std::string::npos&&six.str().find("\"advertised_ds_channel\":1")!=std::string::npos);
    bool invalid=false;try{archer::PassiveScan bad(2);}catch(const std::invalid_argument&){invalid=true;}assert(invalid);
    archer::PassiveScan bounded;
    for(unsigned i=0;i<129;++i){auto f=frame(i);bounded.observe(f,metadata(f));}
    assert(bounded.networks().size()==128&&bounded.overflow()==1);
    std::cout<<"Passive scan grouping, CRC32, channel ambiguity and capacity checks passed\n";
}
