#pragma once
#include "archer/ccmp.hpp"
namespace archer {
#ifdef ARCHER_WPA2_RECOVERED
inline constexpr unsigned secure_first_sequence=28,secure_cleanup_tag=2;
#else
inline constexpr unsigned secure_first_sequence=16,secure_cleanup_tag=0;
#endif
enum class SecureFramePhase:unsigned {authenticate,associate,m2,m4,discover,deauthenticate};
inline unsigned secure_frame_sequence(SecureFramePhase phase){if(unsigned(phase)>5)throw std::invalid_argument("Invalid secure phase");return secure_first_sequence+unsigned(phase);}
inline std::vector<std::uint8_t> secure_tx_packet(SecureFramePhase phase,const std::vector<std::uint8_t>& frame){
 const auto sequence=secure_frame_sequence(phase);const bool data=unsigned(phase)>=2&&unsigned(phase)<=4;
 if(frame.size()<24||frame.size()>4096||management_u16(frame.data()+22)!=(sequence<<4)||
    !std::equal(station_target_ap.begin(),station_target_ap.end(),frame.begin()+4)||
    !std::equal(station_test_mac.begin(),station_test_mac.end(),frame.begin()+10))throw std::invalid_argument("Secure packet identity/sequence mismatch");
 auto packet=special_channel9_grant_probe_packet();packet.resize(64);
 // Source body0 HDR_LLC_LEN is ieee80211_hdrlen/2, including for software crypto.
 channel_put32(packet.data(),data?0x00406480:0x00486480); // DMA0/BE via USB09; MGQ/DMA8 via USB05.
 channel_put32(packet.data()+8,(data?0:0x00240000)|frame.size());// rtw89_core_tx_update_data_info clears sw_mld; management sets it.
 channel_put32(packet.data()+12,(data?0:0x2000)|sequence);
 channel_put32(packet.data()+36,phase==SecureFramePhase::deauthenticate?secure_cleanup_tag<<28:0); // Only final cleanup requests a report in this firmware epoch.
 channel_put32(packet.data()+40,phase==SecureFramePhase::deauthenticate?0x40000000:0);
 packet.insert(packet.end(),frame.begin(),frame.end());return packet;
}
inline std::vector<std::uint8_t> secure_management_packet(SecureFramePhase phase){
 if(phase!=SecureFramePhase::authenticate&&phase!=SecureFramePhase::associate&&phase!=SecureFramePhase::deauthenticate)throw std::invalid_argument("Not a management phase");
 auto kind=phase==SecureFramePhase::authenticate?StationManagementKind::authenticate:phase==SecureFramePhase::associate?StationManagementKind::associate:StationManagementKind::deauthenticate;
 const auto known=station_management_packet(kind);std::vector<std::uint8_t> frame(known.begin()+64,known.end());const auto sequence=secure_frame_sequence(phase);frame[22]=sequence<<4;frame[23]=sequence>>4;
 return secure_tx_packet(phase,frame);
}
inline std::vector<std::uint8_t> secure_eapol_packet(SecureFramePhase phase,const std::vector<std::uint8_t>& eapol){
 if((phase!=SecureFramePhase::m2&&phase!=SecureFramePhase::m4)||eapol.size()<99||eapol[0]!=2||eapol[1]!=3||eapol_be16(eapol.data()+2)!=eapol.size()-4||eapol_be16(eapol.data()+5)!=(phase==SecureFramePhase::m2?0x10a:0x30a))throw std::invalid_argument("Unbound secure EAPOL phase");
 std::vector<std::uint8_t> frame(24);frame[0]=8;frame[1]=1;
 std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+4);std::copy(station_test_mac.begin(),station_test_mac.end(),frame.begin()+10);std::copy(station_target_ap.begin(),station_target_ap.end(),frame.begin()+16);
 const auto seq=secure_frame_sequence(phase);frame[22]=seq<<4;frame[23]=seq>>4;
 frame.insert(frame.end(),{0xaa,0xaa,3,0,0,0,0x88,0x8e});frame.insert(frame.end(),eapol.begin(),eapol.end());return secure_tx_packet(phase,frame);
}
class SecureFrameSendGuard {
 unsigned mask_=0;
public:
 void issue(SecureFramePhase phase,const std::vector<std::uint8_t>& packet){
  const auto i=unsigned(phase);if(i>5||packet.size()<88)throw std::invalid_argument("Invalid secure submission");
  const unsigned bit=1u<<i;
  if(mask_&bit||mask_&32||(i<5&&mask_!=bit-1)||(i==5&&!(mask_&1)))throw std::logic_error("Secure frame order/retry forbidden");
  const std::vector<std::uint8_t> frame(packet.begin()+64,packet.end());
  if(packet!=secure_tx_packet(phase,frame))throw std::invalid_argument("Secure descriptor differs from bound source profile");
  if(i==0||i==1||i==5){if(packet!=secure_management_packet(phase))throw std::invalid_argument("Unbound secure management packet");}
  else if(i==2||i==3){if(frame.size()<32||!std::equal(frame.begin()+24,frame.begin()+32,std::array<std::uint8_t,8>{0xaa,0xaa,3,0,0,0,0x88,0x8e}.begin())||packet!=secure_eapol_packet(phase,{frame.begin()+32,frame.end()}))throw std::invalid_argument("Unbound EAPOL payload");}
  else if(management_u16(frame.data())!=0x4108||frame.size()<40)throw std::invalid_argument("Encrypted data frame required");
  mask_|=bit;
 }
};
inline unsigned internet_checksum(const std::uint8_t* p,std::size_t n){unsigned sum=0;while(n>1){sum+=eapol_be16(p);p+=2;n-=2;}if(n)sum+=unsigned(*p)<<8;while(sum>>16)sum=(sum&0xffff)+(sum>>16);return (~sum)&0xffff;}
inline void network_put32(std::uint8_t* p,std::uint32_t value){for(unsigned i=0;i<4;++i)p[i]=value>>(24-8*i);}
inline std::uint32_t network_u32(const std::uint8_t* p){return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(unsigned(p[2])<<8)|p[3];}
inline std::vector<std::uint8_t> dhcp_discover_msdu(std::uint32_t xid){
 if(!xid)throw std::invalid_argument("Nonzero DHCP transaction required");
 std::vector<std::uint8_t> bootp(240);bootp[0]=1;bootp[1]=1;bootp[2]=6;network_put32(bootp.data()+4,xid);
 std::copy(station_test_mac.begin(),station_test_mac.end(),bootp.begin()+28);network_put32(bootp.data()+236,0x63825363);
 bootp.insert(bootp.end(),{53,1,1,61,7,1});bootp.insert(bootp.end(),station_test_mac.begin(),station_test_mac.end());bootp.insert(bootp.end(),{55,3,1,3,6,255});bootp.resize(300); // BOOTP minimum interoperability padding.
 std::vector<std::uint8_t> ip(28+bootp.size());ip[0]=0x45;eapol_put_be16(ip.data()+2,ip.size());ip[8]=64;ip[9]=17;std::fill(ip.begin()+16,ip.begin()+20,255);
 eapol_put_be16(ip.data()+10,internet_checksum(ip.data(),20));eapol_put_be16(ip.data()+20,68);eapol_put_be16(ip.data()+22,67);eapol_put_be16(ip.data()+24,bootp.size()+8);
 std::copy(bootp.begin(),bootp.end(),ip.begin()+28);std::vector<std::uint8_t> result{0xaa,0xaa,3,0,0,0,8,0};result.insert(result.end(),ip.begin(),ip.end());return result;
}
struct DhcpOffer {std::array<std::uint8_t,4> address{},server{};};
inline std::optional<DhcpOffer> decode_dhcp_offer(const std::vector<std::uint8_t>& msdu,std::uint32_t xid){
 const std::array<std::uint8_t,8> snap{0xaa,0xaa,3,0,0,0,8,0};
 if(msdu.size()<8+20+8+240||!std::equal(snap.begin(),snap.end(),msdu.begin()))return {};
 const auto* ip=msdu.data()+8;const unsigned ihl=(ip[0]&15)*4,total=eapol_be16(ip+2);
 if(ip[0]>>4!=4||ihl<20||ihl>60||msdu.size()<8+total||total<ihl+8+240||ip[9]!=17||eapol_be16(ip+6)&0x3fff||internet_checksum(ip,ihl))return {};
 const auto* udp=ip+ihl;const unsigned length=eapol_be16(udp+4);
 if(eapol_be16(udp)!=67||eapol_be16(udp+2)!=68||length!=total-ihl||length<248)return {};
 if(eapol_be16(udp+6)){
  std::vector<std::uint8_t> check(ip+12,ip+20);check.insert(check.end(),{0,17,std::uint8_t(length>>8),std::uint8_t(length)});check.insert(check.end(),udp,udp+length);
  if(internet_checksum(check.data(),check.size()))return {};
 }
 const auto* b=udp+8;const unsigned size=length-8;
 if(b[0]!=2||b[1]!=1||b[2]!=6||network_u32(b+4)!=xid||!std::equal(station_test_mac.begin(),station_test_mac.end(),b+28)||network_u32(b+236)!=0x63825363)return {};
 bool type=false,server=false,end=false;DhcpOffer offer;std::copy_n(b+16,4,offer.address.begin());
 if(!offer.address[0]||offer.address[0]>=224||offer.address[0]==127)return {};
 for(unsigned at=240;at<size;){const unsigned id=b[at++];if(id==0)continue;if(id==255){end=true;break;}if(at>=size||b[at]>size-at-1)return {};const unsigned n=b[at++];
  if(id==53){if(type||n!=1||b[at]!=2)return {};type=true;}
  if(id==54){if(server||n!=4)return {};std::copy_n(b+at,4,offer.server.begin());server=true;}
  at+=n;
 }
 if(!type||!server||!end||!offer.server[0]||offer.server[0]>=224||offer.server[0]==127)return {};return offer;
}
// Metadata only: plaintext stays in the authenticated, wiping SecretVector.
struct SecureDataMetadata {
 bool group=false,reflected_discover=false;
 unsigned bytes=0,ether_type=0;
 std::optional<DhcpOffer> offer;
};
struct SecureDataObservation {
 unsigned authenticated=0,rejected=0,group=0,pairwise=0,reflected_discover=0,offers=0;
 std::optional<SecureDataMetadata> observe(CcmpSession& ccmp,const std::uint8_t* frame,std::size_t size,std::uint32_t xid){
  // Ignore management, foreign receivers and unprotected data rather than
  // counting them as failed CCMP authentication.
  if(size<24||(management_u16(frame)&0x430c)!=0x4208||
     !std::equal(station_target_ap.begin(),station_target_ap.end(),frame+10)||
     (!(frame[4]&1)&&!std::equal(station_test_mac.begin(),station_test_mac.end(),frame+4)))return {};
  std::optional<SecretVector> plain;
  try{plain.emplace(ccmp.decrypt_from_ap(frame,size));}
  catch(const std::exception&){++rejected;return {};}
  SecureDataMetadata result;result.group=frame[4]&1;result.bytes=plain->bytes.size();
  const std::array<std::uint8_t,6> snap{0xaa,0xaa,3,0,0,0};
  if(plain->bytes.size()>=8&&std::equal(snap.begin(),snap.end(),plain->bytes.begin()))result.ether_type=eapol_be16(plain->bytes.data()+6);
  result.reflected_discover=xid&&plain->bytes==dhcp_discover_msdu(xid)&&
      std::equal(station_test_mac.begin(),station_test_mac.end(),frame+16);
  result.offer=decode_dhcp_offer(plain->bytes,xid);
  ++authenticated;if(result.group)++group;else ++pairwise;
  reflected_discover+=result.reflected_discover;offers+=bool(result.offer);
  return result;
 }
};
inline std::optional<std::vector<std::uint8_t>> target_rsn_eapol(const std::uint8_t* p,std::size_t size){
 if(!station_fcs_valid(p,size)||size<28)return {};size-=4;const auto fc=management_u16(p);
 if(((fc&0x03ff)!=0x0208&&(fc&0x03ff)!=0x0288)||fc&0xc400||management_u16(p+22)&15||
    !std::equal(station_test_mac.begin(),station_test_mac.end(),p+4)||!std::equal(station_target_ap.begin(),station_target_ap.end(),p+10)||!std::equal(station_target_ap.begin(),station_target_ap.end(),p+16))return {};
 const unsigned header=(fc&0x80)?26:24;const std::array<std::uint8_t,8> snap{0xaa,0xaa,3,0,0,0,0x88,0x8e};
 if(size<header+8+99||size>header+8+1131||(header==26&&(p[24]&0x80))||!std::equal(snap.begin(),snap.end(),p+header))return {};
 const auto* e=p+header+8;const auto n=size-header-8;
 if(e[0]!=2||e[1]!=3||eapol_be16(e+2)!=n-4||e[4]!=2||eapol_be16(e+97)!=n-99)return {};
 return std::vector<std::uint8_t>(e,e+n);
}
}
