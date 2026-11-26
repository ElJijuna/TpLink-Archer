#include "archer/ethernet_bridge.hpp"
#include <cassert>
#include <iostream>
#include <vector>
using Bytes=std::vector<std::uint8_t>;
using archer::BridgeStatus;
static const archer::BridgeMac station{2,0,0,0x89,0x22,0xa1},peer{0x50,0x92,0x6a,0x11,6,0x66},broadcast{255,255,255,255,255,255};
Bytes frame(archer::BridgeMac dest,archer::BridgeMac source,unsigned type,unsigned payload=28){
 Bytes b(dest.begin(),dest.end());b.insert(b.end(),source.begin(),source.end());b.push_back(type>>8);b.push_back(type);b.resize(14+payload,0x5a);return b;
}
int main(){
 archer::EthernetBridge bridge(station);std::array<std::uint8_t,1514> output{};
 auto tx=[&](const Bytes& b){return bridge.from_os(b.data(),b.size(),output.data(),output.size());};
 auto rx=[&](const Bytes& b,archer::BridgeMac dest=station,archer::BridgeMac source=peer){return bridge.from_authenticated_ap(b.data(),b.size(),dest,source,output.data(),output.size());};
 const auto arp=frame(broadcast,station,0x0806);auto encoded=tx(arp);
 assert(encoded&&encoded.destination==broadcast&&encoded.bytes==36);
 Bytes msdu(output.begin(),output.begin()+encoded.bytes);
 assert((Bytes(msdu.begin(),msdu.begin()+8)==Bytes{0xaa,0xaa,3,0,0,0,8,6}));
 assert(std::equal(arp.begin()+14,arp.end(),msdu.begin()+8));
 auto reply=rx(msdu);assert(reply&&Bytes(output.begin(),output.begin()+reply.bytes)==frame(station,peer,0x0806));
 reply=rx(msdu,broadcast);assert(reply&&Bytes(output.begin(),output.begin()+reply.bytes)==frame(broadcast,peer,0x0806));
 for(auto type:{0x0800,0x0806,0x86dd}){
  const auto source=frame(peer,station,type,1500);auto result=tx(source);assert(result&&result.bytes==1508);
  Bytes snap(output.begin(),output.begin()+result.bytes);result=rx(snap);assert(result&&result.bytes==1514);
  assert(Bytes(output.begin(),output.end())==frame(station,peer,type,1500));
 }
 assert(tx(frame(peer,station,0x0800,1501)).status==BridgeStatus::invalid_extent);
 assert(tx(Bytes(14)).status==BridgeStatus::invalid_extent);
 assert(tx(frame(peer,peer,0x0800)).status==BridgeStatus::wrong_source);
 assert(!tx(frame({},station,0x0800)));
 for(auto type:{0x888e,0x8100,0x88a8,1500,0x8137})assert(tx(frame(peer,station,type)).status==BridgeStatus::unsupported_protocol);
 assert(!rx(msdu,peer,peer));assert(!rx(msdu,station,station));assert(!rx(msdu,station,broadcast));assert(!rx(msdu,station,{}));
 for(unsigned n=0;n<9;++n)assert(!rx(Bytes(n)));
 for(unsigned i=0;i<6;++i){auto bad=msdu;bad[i]^=1;assert(rx(bad).status==BridgeStatus::unsupported_snap);}
 auto bad=msdu;bad[6]=0x88;bad[7]=0x8e;assert(rx(bad).status==BridgeStatus::unsupported_protocol);
 assert(!archer::EthernetBridge(broadcast).from_os(arp.data(),arp.size(),output.data(),output.size()));
 assert(!archer::EthernetBridge(station,1501).from_os(arp.data(),arp.size(),output.data(),output.size()));
 assert(!archer::EthernetBridge(station,0).from_os(arp.data(),arp.size(),output.data(),output.size()));
 // Failures cannot partially publish output; null, short and overlapping buffers.
 output.fill(0x55);auto before=output;
 assert(bridge.from_os(arp.data(),arp.size(),output.data(),35).status==BridgeStatus::short_output);assert(output==before);
 assert(!bridge.from_os(nullptr,arp.size(),output.data(),output.size()));
 assert(!bridge.from_os(arp.data(),arp.size(),nullptr,2000));
 auto inplace=arp;auto saved=inplace;
 assert(bridge.from_os(inplace.data(),inplace.size(),inplace.data(),inplace.size()).status==BridgeStatus::overlapping_buffers);assert(inplace==saved);
 assert(bridge.from_authenticated_ap(msdu.data(),msdu.size(),station,peer,output.data(),41).status==BridgeStatus::short_output);assert(output==before);
 assert(!bridge.from_authenticated_ap(nullptr,36,station,peer,output.data(),output.size()));
 auto copy=msdu;copy.resize(100);assert(bridge.from_authenticated_ap(copy.data(),msdu.size(),station,peer,copy.data()+1,99).status==BridgeStatus::overlapping_buffers);
 std::cout<<"Ethernet bridge: SNAP, addressing, reflection, MTU, buffer bounds and EAPOL separation passed\n";
}
