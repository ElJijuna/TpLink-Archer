#include "archer/interface_role.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::invalid_argument&){yes=true;}assert(yes);}
int main(){
 using namespace archer;
 const std::vector<std::uint8_t> create{0x21,4,0,7,12,0x80,0,0,0,0x20,0,0};
 const std::vector<std::uint8_t> remove{0x21,4,0,8,12,0x80,0,0,0,0x24,0,0};
 assert(encode_unlinked_station_role(7,0,0,StationRoleAction::create)==create);
 assert(encode_unlinked_station_role(8,0,0,StationRoleAction::remove)==remove);
 auto max=encode_unlinked_station_role(255,255,4,StationRoleAction::create);
 assert(channel_u32(max.data()+8)==0x2020ff);
 rejects([]{encode_unlinked_station_role(256,0,0,StationRoleAction::create);});
 rejects([]{encode_unlinked_station_role(0,256,0,StationRoleAction::create);});
 rejects([]{encode_unlinked_station_role(0,0,5,StationRoleAction::create);});
 rejects([]{encode_unlinked_station_role(0,0,0,static_cast<StationRoleAction>(7));});
 std::cout<<"Offline station role golden frames and bounds passed\n";
}
