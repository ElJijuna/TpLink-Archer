#include "archer/eapol_m1.hpp"
#include <cassert>
#include <map>
#include <iostream>
#include <fstream>
using namespace archer;
void fcs(std::vector<std::uint8_t>& p){auto n=p.size();p.resize(n+4);channel_put32(p.data()+n,ieee_fcs(p.data(),n));}
std::vector<std::uint8_t> sample(bool qos=false){
 std::vector<std::uint8_t> p(qos?26:24);p[0]=qos?0x88:8;p[1]=2;
 std::copy(station_test_mac.begin(),station_test_mac.end(),p.begin()+4);
 std::copy(station_target_ap.begin(),station_target_ap.end(),p.begin()+10);std::copy(station_target_ap.begin(),station_target_ap.end(),p.begin()+16);
 p.insert(p.end(),{0xaa,0xaa,3,0,0,0,0x88,0x8e,2,3,0,95});
 auto at=p.size();p.resize(at+95);p[at]=2;p[at+2]=0x8a;p[at+4]=16;p[at+12]=1;
 for(unsigned i=0;i<32;++i)p[at+13+i]=i+1;
 fcs(p);return p;
}
struct Bus:RegisterBus {
 std::map<unsigned,unsigned> values{{0x10398,0},{0x1142c,0xa5000000}};unsigned writes=0;
 unsigned read(unsigned a,unsigned w)override{assert(w==4);return values.at(a);}
 void write(unsigned a,unsigned w,unsigned v)override{assert(w==4&&a==0x1142c);values[a]=v;++writes;}
 void pause()override{}
};
int main(){
 std::ifstream captured("tests/fixtures/special-eapol-m1-stage104.bin",std::ios::binary);
 std::vector<std::uint8_t> fixture((std::istreambuf_iterator<char>(captured)),{});
 const auto real=decode_target_eapol_m1(fixture.data(),fixture.size());
 assert(fixture.size()==135&&real&&real->replay[7]==1&&!real->key_data_bytes);

 for(bool qos:{false,true}){
  auto p=sample(qos);auto m=decode_target_eapol_m1(p.data(),p.size());assert(m&&m->replay[7]==1&&m->nonce[31]==32&&!m->key_data_bytes);
  for(std::size_t n=0;n<p.size();++n)assert(!decode_target_eapol_m1(p.data(),n));
  // Mutations have fresh valid CRC, so rejection tests the protocol boundaries.
  for(unsigned at:{1u,4u,10u,16u,22u,unsigned(qos?26:24)+7,unsigned(qos?26:24)+9,
                   unsigned(qos?26:24)+11,unsigned(qos?26:24)+14,unsigned(qos?26:24)+15,
                   unsigned(qos?26:24)+12+77,unsigned(qos?26:24)+12+94}){
   auto bad=p;bad.resize(bad.size()-4);bad[at]^=1;fcs(bad);assert(!decode_target_eapol_m1(bad.data(),bad.size()));
  }
 }
 for(unsigned round=0;round<2;++round){StationManagementSendGuard guard;
  for(auto kind:{StationManagementKind::authenticate,StationManagementKind::associate,StationManagementKind::deauthenticate}){
   auto p=station_management_packet(kind,round);assert((channel_u32(p.data()+36)>>28)==unsigned(kind)+3*round);guard.issue(kind,p,round);
  }
 }
 StationManagementSendGuard mixed;mixed.issue(StationManagementKind::authenticate,station_management_packet(StationManagementKind::authenticate,1),1);
 bool bad=false;try{mixed.issue(StationManagementKind::associate,station_management_packet(StationManagementKind::associate));}catch(...){bad=true;}assert(bad);
 Bus bus;with_station_data_rx_window(bus,[&]{assert(bus.values[0x1142c]==0xa5000101);});assert(bus.writes==2&&bus.values[0x1142c]==0xa5000000);
 bad=false;try{with_station_data_rx_window(bus,[]{throw std::runtime_error("stop");});}catch(...){bad=true;}assert(bad&&bus.values[0x1142c]==0xa5000000);
 std::cout<<"Strict RSN M1, normal/QoS/FCS/source/extent guards, unique tags13-15 and data-filter restoration passed\n";
}
