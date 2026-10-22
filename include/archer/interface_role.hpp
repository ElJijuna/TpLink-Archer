#pragma once
#include "archer/firmware_channel.hpp"
namespace archer {
enum class StationRoleAction : unsigned { create=0, remove=1 };
enum class StationCapability : unsigned { legacy=0, he=1, eht=2 };
// Source-derived codec. No I/O here; the context executor owns lifecycle/ACKs.
inline std::vector<std::uint8_t> encode_unlinked_station_role(unsigned sequence,
    unsigned mac_id,unsigned port,StationRoleAction action){
    if(mac_id>255 || port>4 || (action!=StationRoleAction::create && action!=StationRoleAction::remove))
        throw std::invalid_argument("Unsupported station role fields");
    std::vector<std::uint8_t> payload(4);
    // CLIENT self-role0, STATION Wi-Fi role1, MAC0; extended MACID remains0.
    channel_put32(payload.data(),mac_id|(unsigned(action)<<10)|(1u<<13)|(port<<19));
    return encode_h2c(1,8,4,sequence,payload,false,true);
}
// JOIN_INFO v1 for one designated non-MLD station link, MAC0, NO_LINK.
// Source dis_conn=true initializes the context; it does not associate a network.
inline std::vector<std::uint8_t> encode_unlinked_station_join(unsigned sequence,
    unsigned mac_id,unsigned port,unsigned wmm,StationCapability capability){
    if(mac_id>255 || port>4 || wmm>3 || unsigned(capability)>2)
        throw std::invalid_argument("Unsupported unlinked JOIN_INFO fields");
    std::vector<std::uint8_t> payload(12);
    channel_put32(payload.data(),mac_id|0x100u|(wmm<<10)|(port<<21)|(1u<<26));
    // MAIN_MACID=self, MLSR=1, EML padding256us=4, transition256us=5.
    // No MLD/EMLSR/NSTR/initial power-save; all reserved/extension bits zero.
    channel_put32(payload.data()+4,unsigned(capability)|(mac_id<<4)|0x2c1000u);
    return encode_h2c(1,8,0,sequence,payload,false,true);
}
}
