#include "archer/beacon.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::invalid_argument&){yes=true;}assert(yes);}
int main(){
 std::vector<std::uint8_t>b(36);b[0]=0x80;b[32]=100;
 b.insert(b.end(),{0,3,'a',0,'b',3,1,6,221,0});
 auto parse=[&]{return archer::parse_beacon(b.data(),b.size());};
 auto r=parse();assert(!r.probe_response&&r.interval_tu==100&&r.ssid.size()==3&&r.ssid[1]==0&&r.advertised_ds_channel==6);
 b[0]=0x50;assert(parse().probe_response);b[0]=0x80;
 auto good=b;
 rejects([&]{archer::parse_beacon(nullptr,36);});
 for(std::size_t n=0;n<36;++n)rejects([&]{archer::parse_beacon(b.data(),n);});
 b.push_back(1);rejects(parse);b=good;b.back()=1;rejects(parse);b=good;
 b.insert(b.end(),{0,0});rejects(parse);b=good;
 b.insert(b.end(),{3,1,6});rejects(parse);b=good;
 b[37]=33;rejects(parse);b=good;
 for(unsigned flag:{1u,2u,4u,64u,128u}){b[1]=flag;rejects(parse);}b=good;
 b[22]=1;rejects(parse);b=good;b[0]=0x88;rejects(parse);b=good;
 std::vector<std::uint8_t>empty(38);empty[0]=0x80;assert(archer::parse_beacon(empty.data(),empty.size()).ssid.empty());
 archer::BeRxMetadata m;m.packet.payload_offset=0;m.packet.payload_bytes=good.size();m.header_converted=false;
 rejects([&]{archer::decode_rx_beacon(good,m);});m.fcs_policy=archer::RxFcsPolicy::stripped;
 assert(archer::decode_rx_beacon(good,m).ssid.size()==3);
 auto appended=good;appended.insert(appended.end(),4,0);
 m.fcs_policy=archer::RxFcsPolicy::appended;m.packet.payload_bytes=appended.size();
 assert(archer::decode_rx_beacon(appended,m).ssid.size()==3);
 m.fcs_policy=archer::RxFcsPolicy::stripped;m.packet.payload_bytes=good.size();
 m.crc_error=true;rejects([&]{archer::decode_rx_beacon(good,m);});m.crc_error=false;
 m.packet.payload_offset=good.size()+1;rejects([&]{archer::decode_rx_beacon(good,m);});
 std::cout<<"Beacon parser tests passed\n";
}
