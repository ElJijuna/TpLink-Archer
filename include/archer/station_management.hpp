#pragma once
#include "archer/single_probe.hpp"
#include "archer/passive_scan.hpp"
namespace archer {
inline const std::array<std::uint8_t,6> station_test_mac{2,0,0,0x89,0x22,0xa1};
inline const std::array<std::uint8_t,6> station_target_ap{0x50,0x92,0x6a,0x11,0x06,0x66};
inline unsigned management_u16(const std::uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline bool station_fcs_valid(const std::uint8_t* p,std::size_t n){return n>=28&&ieee_fcs(p,n-4)==channel_u32(p+n-4);}
enum class StationManagementKind {authenticate=10,associate=11,deauthenticate=12};
// Narrow legacy WPA2-PSK/CCMP management experiment, not a supplicant or linked
// firmware data path. No credential, key material, EAPOL transmission or retry.
inline std::vector<std::uint8_t> station_management_packet(StationManagementKind kind){
 const unsigned tag=unsigned(kind);
 if(tag<10||tag>12)throw std::invalid_argument("Invalid station management phase");
 std::vector<std::uint8_t> frame(24);
 frame[0]=kind==StationManagementKind::authenticate?0xb0:kind==StationManagementKind::associate?0:0xc0;
 std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+4);
 std::copy(station_test_mac.begin(),station_test_mac.end(),frame.begin()+10);
 std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+16);
 frame[22]=tag<<4;
 if(kind==StationManagementKind::authenticate)frame.insert(frame.end(),{0,0,1,0,0,0});
 else if(kind==StationManagementKind::deauthenticate)frame.insert(frame.end(),{3,0});
 else {
  // ESS + Privacy, listen interval10, four supported CCK basic rates.
  frame.insert(frame.end(),{0x11,0,10,0,0,8,'E','s','p','e','c','i','a','l',1,4,0x82,0x84,0x8b,0x96});
  // RSN1, group CCMP, one pairwise CCMP, one PSK AKM, no PMF capability.
  frame.insert(frame.end(),{48,20,1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,2,0,0});
 }
 auto packet=special_channel9_grant_probe_packet();packet.resize(64);
 channel_put32(packet.data()+8,0x00240000|frame.size());
 channel_put32(packet.data()+12,0x2000|tag);channel_put32(packet.data()+36,tag<<28);
 packet.insert(packet.end(),frame.begin(),frame.end());return packet;
}
class StationManagementSendGuard {
 unsigned mask_=0;
public:
 void issue(StationManagementKind kind,const std::vector<std::uint8_t>& frame){
  const unsigned tag=unsigned(kind);
  if(tag<10||tag>12||frame!=station_management_packet(kind))throw std::invalid_argument("Unbound station frame");
  const unsigned bit=1u<<(tag-10);
  if((mask_&bit)||(mask_&4)||(tag==10&&mask_)||(tag!=10&&!(mask_&1)))
   throw std::logic_error("Station management order/retry forbidden");
  mask_|=bit; // Consumed before a transport call, including uncertain writes.
 }
};
struct StationManagementReply {StationManagementKind kind;unsigned status=0,aid=0;};
inline std::optional<StationManagementReply> decode_station_reply(const std::uint8_t* p,std::size_t n){
 if(!station_fcs_valid(p,n))return {};
 n-=4;
 if(!std::equal(station_test_mac.begin(),station_test_mac.end(),p+4)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+10)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+16))return {};
 const auto fc=management_u16(p);
 if(fc&0xc307)return {}; // No protected/order/DS/version/non-management fields.
 if((fc&0xfc)==0xb0){
  if(n<30||management_u16(p+24)!=0||management_u16(p+26)!=2)return {};
  for(std::size_t at=30;at<n;){if(n-at<2||p[at+1]>n-at-2)return {};at+=2+p[at+1];}
  return StationManagementReply{StationManagementKind::authenticate,management_u16(p+28),0};
 }
 if((fc&0xfc)==0x10){
  if(n<30)return {};
  for(std::size_t at=30;at<n;){if(n-at<2||p[at+1]>n-at-2)return {};at+=2+p[at+1];}
  const auto status=management_u16(p+26),raw_aid=management_u16(p+28),aid=raw_aid&0x3fff;
  if(!status&&((raw_aid&0xc000)!=0xc000||!aid||aid>2007))return {};
  return StationManagementReply{StationManagementKind::associate,status,status?0:aid};
 }
 return {};
}
// Caller supplies a complete target probe response including software-checked FCS.
// Admit only the already observed RSN profile; do not infer a secure connection.
inline void validate_station_target_profile(const std::uint8_t* p,std::size_t n){
 if(!station_fcs_valid(p,n)||n<40||(p[0]&0xfc)!=0x50||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+10)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),p+16)||
    !std::equal(station_test_mac.begin(),station_test_mac.end(),p+4)||
    (management_u16(p+34)&0x11)!=0x11)throw std::invalid_argument("Verified target probe response required");
 bool ssid=false,rsn=false,channel=false;unsigned rates=0;n-=4;
 const std::vector<std::uint8_t> expected_rsn{1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,4,1,0,0,0x0f,0xac,2};
 for(std::size_t at=36;at<n;){
  if(n-at<2||p[at+1]>n-at-2)throw std::invalid_argument("Truncated target IE");
  const unsigned id=p[at],len=p[at+1];const auto* value=p+at+2;
  if(id==0){if(ssid||len!=8||!std::equal(value,value+8,"Especial"))throw std::invalid_argument("Target SSID mismatch");ssid=true;}
  if(id==3){if(channel||len!=1||*value!=9)throw std::invalid_argument("Target channel mismatch");channel=true;}
  if(id==1||id==50)for(unsigned j=0;j<len;++j)if(value[j]&128){switch(value[j]&127){case 2:rates|=1;break;case 4:rates|=2;break;case 11:rates|=4;break;case 22:rates|=8;break;default:throw std::invalid_argument("Unsupported basic rate");}}
  if(id==48){if(rsn||len!=20||!std::equal(expected_rsn.begin(),expected_rsn.end(),value)||(management_u16(value+18)&0xc0))throw std::invalid_argument("Unsupported target RSN/PMF profile");rsn=true;}
  at+=2+len;
 }
 if(!ssid||!rsn||!channel||rates!=15)throw std::invalid_argument("Incomplete target profile");
}
}
