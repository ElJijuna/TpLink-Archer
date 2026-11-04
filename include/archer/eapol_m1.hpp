#pragma once
#include "archer/station_management.hpp"
#include "archer/channel_bus.hpp"
namespace archer {
struct EapolM1 {std::array<std::uint8_t,8> replay{};std::array<std::uint8_t,32> nonce{};unsigned key_data_bytes=0;};
inline unsigned eapol_be16(const std::uint8_t* p){return (unsigned(p[0])<<8)|p[1];}
// A strict plaintext RSN-PSK/CCMP M1 observation. This does not verify a MIC,
// authenticate the peer, derive a key, transmit EAPOL or authorize data.
inline std::optional<EapolM1> decode_target_eapol_m1(const std::uint8_t* p,std::size_t n){
 if(!station_fcs_valid(p,n))return {};n-=4;
 if(n<24)return {};
 const auto fc=management_u16(p);
 if((fc&0x03ff)!=0x0208&&(fc&0x03ff)!=0x0288)return {};
 if(fc&0xc400||management_u16(p+22)&15)return {}; // Protected/order/fragmented.
 if(!std::equal(station_test_mac.begin(),station_test_mac.end(),p+4)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+10)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+16))return {};
 const unsigned header=(fc&0x80)?26:24;
 if(n<header+8+4+95||(header==26&&(p[24]&0x80)))return {};
 const std::array<std::uint8_t,8> snap{0xaa,0xaa,3,0,0,0,0x88,0x8e};
 if(!std::equal(snap.begin(),snap.end(),p+header))return {};
 const auto* e=p+header+8;const auto extent=n-header-8;
 if((e[0]!=1&&e[0]!=2)||e[1]!=3||eapol_be16(e+2)!=extent-4)return {};
 const auto* key=e+4;const auto length=extent-4;
 if(key[0]!=2||eapol_be16(key+1)!=0x008a||eapol_be16(key+3)!=16||
    eapol_be16(key+93)!=length-95)return {};
 if(std::any_of(key+77,key+93,[](auto b){return b!=0;})||
    std::all_of(key+13,key+45,[](auto b){return b==0;}))return {};
 EapolM1 result;std::copy(key+5,key+13,result.replay.begin());std::copy(key+13,key+45,result.nonce.begin());result.key_data_bytes=length-95;return result;
}
inline void with_station_data_rx_window(RegisterBus& bus,const std::function<void()>& action){
 if(channel_bus_read(bus,0x10398)&0x3ffff)throw std::runtime_error("Data RX contrast requires TX pause");
 const auto saved=channel_bus_read(bus,0x1142c);
 if(saved&0xffff)throw std::runtime_error("Data RX contrast requires management-only baseline");
 std::exception_ptr error;
 try{channel_bus_write(bus,0x1142c,0xffff,0x101);action();}catch(...){error=std::current_exception();}
 channel_bus_write(bus,0x1142c,0xffff,saved&0xffff);
 if(error)std::rethrow_exception(error);
}
}
