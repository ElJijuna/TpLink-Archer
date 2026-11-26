#include "archer/unicast_icmp.hpp"
#include <cassert>
#include <iostream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
void checksums(std::vector<std::uint8_t>& b){b[18]=b[19]=b[30]=b[31]=0;eapol_put_be16(b.data()+18,internet_checksum(b.data()+8,20));eapol_put_be16(b.data()+30,internet_checksum(b.data()+28,24));}
int main(){
 DhcpOffer lease;lease.address={192,0,2,10};lease.router={192,0,2,1};lease.subnet={255,255,255,0};lease.lease_seconds=3600;
 const std::array<std::uint8_t,6> peer{2,1,2,3,4,5};std::array<std::uint8_t,16> cookie{};for(unsigned i=0;i<16;++i)cookie[i]=i+1;
 GatewayIcmpProbe probe(lease,peer,123,cookie);auto request=probe.issue_request();assert(request.size()==52&&request[28]==8&&internet_checksum(request.data()+8,20)==0&&internet_checksum(request.data()+28,24)==0);rejects([&]{probe.issue_request();});
 auto reply=request;std::copy(lease.router.begin(),lease.router.end(),reply.begin()+20);std::copy(lease.address.begin(),lease.address.end(),reply.begin()+24);reply[28]=0;checksums(reply);
 assert(!probe.accept_authenticated_reply(reply,false,peer));auto wrong=peer;wrong[1]^=1;assert(!probe.accept_authenticated_reply(reply,true,wrong));
 for(unsigned at:{0u,8u,10u,14u,17u,20u,24u,28u,29u,32u,34u,36u}){auto bad=reply;bad[at]^=1;checksums(bad);assert(!probe.accept_authenticated_reply(bad,true,peer));}
 auto bad=reply;bad[40]^=1;assert(!probe.accept_authenticated_reply(bad,true,peer));
 for(unsigned n=0;n<52;++n){bad=reply;bad.resize(n);assert(!probe.accept_authenticated_reply(bad,true,peer));}
 GatewayIcmpProbe not_issued(lease,peer,123,cookie);assert(!not_issued.accept_authenticated_reply(reply,true,peer));
 assert(probe.accept_authenticated_reply(reply,true,peer));assert(probe.matched());assert(!probe.accept_authenticated_reply(reply,true,peer));
 auto invalid=lease;invalid.router={192,0,3,1};rejects([&]{GatewayIcmpProbe p(invalid,peer,123,cookie);});rejects([&]{GatewayIcmpProbe p(lease,peer,65536,cookie);});
 std::cout<<"Single gateway Echo, matching identities/cookie/checksums, fragments and retry guards passed\n";
}
