#pragma once
#include "archer/firmware_channel.hpp"
namespace archer {
enum class StationRoleAction : unsigned { create=0, remove=1 };
// Offline source reconstruction only. Does not initialize port, CAM, tables,
// join-info or ACK lifecycle, and is not wired to a hardware sender.
inline std::vector<std::uint8_t> encode_unlinked_station_role(unsigned sequence,
    unsigned mac_id,unsigned port,StationRoleAction action){
    if(mac_id>255 || port>4 || (action!=StationRoleAction::create && action!=StationRoleAction::remove))
        throw std::invalid_argument("Unsupported station role fields");
    std::vector<std::uint8_t> payload(4);
    // CLIENT self-role0, STATION Wi-Fi role1, MAC0; extended MACID remains0.
    channel_put32(payload.data(),mac_id|(unsigned(action)<<10)|(1u<<13)|(port<<19));
    return encode_h2c(1,8,4,sequence,payload,false,true);
}
}
