#include "archer/dhcp_selecting.hpp"
#include <cassert>
#include <iostream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
std::vector<std::uint8_t> reply(unsigned type,std::uint32_t xid=0x12345678){
 auto b=dhcp_discover_msdu(xid,true);auto* ip=b.data()+8;auto* udp=ip+20;auto* boot=udp+8;
 boot[0]=2;boot[16]=192;boot[17]=0;boot[18]=2;boot[19]=10;
 std::fill(b.begin()+276,b.end(),0);
 const std::vector<std::uint8_t> options{53,1,std::uint8_t(type),54,4,192,0,2,1,51,4,0,0,14,16,255};
 std::copy(options.begin(),options.end(),b.begin()+276);
 eapol_put_be16(udp,67);eapol_put_be16(udp+2,68);ip[12]=192;ip[14]=2;ip[15]=1;ip[10]=ip[11]=0;eapol_put_be16(ip+10,internet_checksum(ip,20));return b;
}
int main(){
 assert(dhcp_discover_backoff_ms(0,0)==3000&&dhcp_discover_backoff_ms(0,2000)==5000);
 assert(dhcp_discover_backoff_ms(1,0)==7000&&dhcp_discover_backoff_ms(1,2000)==9000);
 rejects([]{dhcp_discover_backoff_ms(2,0);});rejects([]{dhcp_discover_backoff_ms(0,2001);});
 auto initial=dhcp_discover_msdu(1,true),retry=dhcp_discover_msdu(1,true,4);assert(eapol_be16(retry.data()+44)==4);retry[45]=0;assert(initial==retry);
 rejects([]{dhcp_discover_msdu(1,true,65536);});
 const auto offer=reply(2),ack=reply(5);DhcpSelectingExchange exchange(0x12345678);
 rejects([]{DhcpSelectingExchange bad(0);});rejects([&]{exchange.issue_request();});rejects([&]{exchange.issue_release();});
 assert(!exchange.accept_authenticated_ack(ack));assert(!exchange.accept_authenticated_offer(reply(2,1)));
 assert(exchange.accept_authenticated_offer(offer));assert(!exchange.accept_authenticated_offer(offer));
 auto request=exchange.issue_request(8);assert(eapol_be16(request.data()+44)==8);assert(request.size()==336&&request[278]==3&&request[279]==50&&request[285]==54);
 assert(eapol_be16(request.data()+46)==0x8000&&network_u32(request.data()+48)==0&&internet_checksum(request.data()+8,20)==0);
 assert(std::equal(request.begin()+281,request.begin()+285,ack.begin()+52));
 rejects([&]{exchange.issue_request();});assert(!exchange.accept_authenticated_ack(offer));assert(!exchange.accept_authenticated_ack(reply(6)));
 for(unsigned at:{40u,64u,55u,284u}){auto bad=ack;bad[at]^=1;assert(!exchange.accept_authenticated_ack(bad));}
 for(std::size_t n=0;n<ack.size()-1;++n){auto bad=ack;bad.resize(n);assert(!exchange.accept_authenticated_ack(bad));}
 auto missing=ack;missing[285]=255;assert(!exchange.accept_authenticated_ack(missing));
 auto zero=ack;std::fill(zero.begin()+287,zero.begin()+291,0);assert(!exchange.accept_authenticated_ack(zero));
 assert(!exchange.lease_acknowledged());assert(exchange.accept_authenticated_ack(ack));assert(exchange.lease_acknowledged());
 assert(!exchange.accept_authenticated_ack(ack));auto release=exchange.issue_release();
 assert(release[278]==7&&release[279]==54&&eapol_be16(release.data()+46)==0);
 assert(std::equal(release.begin()+48,release.begin()+52,ack.begin()+52));
 assert(std::equal(release.begin()+20,release.begin()+24,ack.begin()+52));
 assert(internet_checksum(release.data()+8,20)==0);rejects([&]{exchange.issue_release();});
 // A duplicate critical option must not replace the first one.
 auto duplicate=ack;duplicate[291]=53;duplicate[292]=1;duplicate[293]=5;duplicate[294]=255;
 assert(!decode_dhcp_reply(duplicate,0x12345678,5));
 DhcpSelectingExchange retried(0x12345678);assert(retried.accept_authenticated_offer(offer));retried.issue_request(3);
 auto second=retried.issue_request_retry(7);assert(eapol_be16(second.data()+44)==7);
 rejects([&]{retried.issue_request_retry(8);});assert(retried.accept_authenticated_ack(ack));rejects([&]{retried.issue_request_retry(9);});
 DhcpSelectingExchange negative(0x12345678);assert(negative.accept_authenticated_offer(offer));negative.issue_request(3);
 auto nak=reply(6);std::fill(nak.begin()+52,nak.begin()+56,0);
 assert(!negative.accept_authenticated_nak(reply(6,1)));assert(negative.accept_authenticated_nak(nak));
 rejects([&]{negative.issue_request_retry(7);});assert(!negative.accept_authenticated_ack(ack));assert(!negative.lease_acknowledged());
 DhcpSelectingExchange early(0x12345678);rejects([&]{early.issue_request_retry(7);});
 assert(early.accept_authenticated_offer(offer));early.issue_request(8);rejects([&]{early.issue_request_retry(7);});
 std::cout<<"DHCP selection, matching ACK/lease, REQUEST/RELEASE fields and no-retry state passed\n";
}
