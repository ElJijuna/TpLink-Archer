#include "archer/station_management.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace archer;
void append_fcs(std::vector<std::uint8_t>& f){auto crc=ieee_fcs(f.data(),f.size());auto n=f.size();f.resize(n+4);channel_put32(f.data()+n,crc);}
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int main(){
 auto auth=station_management_packet(StationManagementKind::authenticate),assoc=station_management_packet(StationManagementKind::associate),bye=station_management_packet(StationManagementKind::deauthenticate);
 assert(auth.size()==94&&assoc.size()==130&&bye.size()==90);
 for(const auto& p:{auth,assoc,bye}){assert((channel_u32(p.data()+8)&0x3fff)==p.size()-64);assert(channel_u32(p.data()+48)==0x80000000);assert(p[68]==0x50&&p[74]==2);}
 StationManagementSendGuard guard;rejects([&]{guard.issue(StationManagementKind::associate,assoc);});
 guard.issue(StationManagementKind::authenticate,auth);rejects([&]{guard.issue(StationManagementKind::authenticate,auth);});
 guard.issue(StationManagementKind::associate,assoc);guard.issue(StationManagementKind::deauthenticate,bye);rejects([&]{guard.issue(StationManagementKind::associate,assoc);});
 StationManagementSendGuard early;early.issue(StationManagementKind::authenticate,auth);early.issue(StationManagementKind::deauthenticate,bye);
 auto reply=std::vector<std::uint8_t>(auth.begin()+64,auth.end());std::copy(station_test_mac.begin(),station_test_mac.end(),reply.begin()+4);std::copy(station_target_ap.begin(),station_target_ap.end(),reply.begin()+10);reply[26]=2;append_fcs(reply);
 assert(decode_station_reply(reply.data(),reply.size())->status==0);
 auto bad=reply;bad[4]^=1;assert(!decode_station_reply(bad.data(),bad.size()));
 bad=reply;bad[28]=13;bad.resize(bad.size()-4);append_fcs(bad);assert(decode_station_reply(bad.data(),bad.size())->status==13);
 reply.resize(30);reply[0]=0x10;reply[24]=0x11;reply[26]=0;reply[27]=0;reply[28]=7;reply[29]=0xc0;append_fcs(reply);
 assert(decode_station_reply(reply.data(),reply.size())->aid==7);
 reply[29]=0;reply.resize(30);append_fcs(reply);assert(!decode_station_reply(reply.data(),reply.size()));
 std::ifstream fixture("tests/fixtures/special-probe-response-stage102.bin",std::ios::binary);std::vector<std::uint8_t> observed((std::istreambuf_iterator<char>(fixture)),{});assert(!observed.empty());validate_station_target_profile(observed.data(),observed.size());
 for(std::size_t n=0;n<observed.size();++n)rejects([&]{validate_station_target_profile(observed.data(),n);});
 observed.back()^=1;rejects([&]{validate_station_target_profile(observed.data(),observed.size());});
 std::cout<<"Station auth/association/deauth packets, one-shot order, FCS/source/AID and observed RSN profile passed\n";
}
