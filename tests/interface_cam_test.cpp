#include "archer/interface_cam.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::invalid_argument&){yes=true;}assert(yes);}
int main(){
 using namespace archer;
 UnlinkedCamContext c{{2,0x11,0x22,0x33,0x44,0x55},0,0,0,0};
 const std::vector<std::uint8_t> expected{
 0x19,0,0,10,68,0x80,0,0,
 0,0,0,0, 0,0,0x20,0, 1,0,0x13,0, 0,0,0,0,
 2,0x11,0x22,0x33, 0x44,0x55,0,0, 0,0,0,0, 0,0,0,0,
 0,0,0,0, 0,0,2,0, 0,0,0,0, 0,0,0,0,
 0,0,8,0, 0xfd,0,0,0, 0,0,0,0};
 assert(encode_unlinked_cam_v0(10,c,StationRoleAction::create)==expected);
 auto removed=expected;removed[16]=0;removed[60]=0xfc;
 assert(encode_unlinked_cam_v0(10,c,StationRoleAction::remove)==removed);
 c.address_index=127;c.bssid_index=19;c.mac_id=255;c.port=4;
 auto upper=encode_unlinked_cam_v0(255,c,StationRoleAction::create);
 assert(channel_u32(upper.data()+12)==0x20007f&&channel_u32(upper.data()+20)==19);
 assert(channel_u32(upper.data()+40)==0x24ff&&channel_u32(upper.data()+56)==0x80013);
 c.port=5;rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});c.port=4;
 c.address_index=128;rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});c.address_index=0;
 c.bssid_index=20;rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});c.bssid_index=0;
 c.mac_id=256;rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});c.mac_id=0;
 c.station_address[0]=3;rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});
 c.station_address={};rejects([&]{encode_unlinked_cam_v0(0,c,StationRoleAction::create);});
 std::cout<<"CAM V0 golden create/invalidation and bounds passed\n";
}
