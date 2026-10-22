#pragma once
#include "archer/interface_role.hpp"
#include <array>
namespace archer {
struct UnlinkedCamContext {
    std::array<std::uint8_t,6> station_address{};
    unsigned address_index=0,bssid_index=0,mac_id=0,port=0;
};
// Source RTL8922A addrcam_ver0 and firmware ADDR_CAM_V0 (<0.35.84).
// Fixed NO_LINK/PHY0, zero BSSID, no keys/WoW/trigger/AID. Offline only.
inline std::vector<std::uint8_t> encode_unlinked_cam_v0(unsigned sequence,
    const UnlinkedCamContext& c,StationRoleAction action){
    bool any=false;unsigned hash=0;
    for(auto byte:c.station_address){any|=byte!=0;hash^=byte;}
    if(!any || (c.station_address[0]&1) || c.address_index>=128 || c.bssid_index>=20 ||
       c.mac_id>255 || c.port>4 || (action!=StationRoleAction::create&&action!=StationRoleAction::remove))
        throw std::invalid_argument("Unsupported unlinked CAM V0 context");
    const unsigned valid=action==StationRoleAction::create?1:0;
    std::vector<std::uint8_t> payload(60);
    const auto word=[&](unsigned index,unsigned value){channel_put32(payload.data()+4*index,value);};
    word(1,c.address_index|(0x20u<<16));
    word(2,valid|(hash<<16));word(3,c.bssid_index);
    word(4,channel_u32(c.station_address.data()));
    word(5,c.station_address[4]|(unsigned(c.station_address[5])<<8));
    word(8,c.mac_id|(c.port<<8)|(c.port<<11));word(9,2u<<16);
    word(12,c.bssid_index|(8u<<16));word(13,valid|0xfcu);
    return encode_h2c(1,6,0,sequence,payload,false,true);
}
}
