#include "archer/storage_eject.hpp"
#include <cassert>
#include <iostream>
int main(){
    const auto a=archer::installer_eject_cbw(false),e=archer::installer_eject_cbw(true);
    assert(a[14]==6 && a[15]==0x1e && a[19]==0 && e[15]==0x1b && e[19]==2);
    for(unsigned i=8;i<14;++i)assert(a[i]==0 && e[i]==0);
    std::array<std::uint8_t,13> csw{0x55,0x53,0x42,0x53,0x97,0x65,0x43,0x21,0,0,0,0,0};
    assert(archer::installer_csw(csw,true)==0);
    csw[12]=1;assert(archer::installer_csw(csw,true)==1);
    std::array<std::uint8_t,512> packet{};
    std::copy(csw.begin(),csw.end(),packet.begin());
    assert(archer::installer_csw_packet(packet.data(),13,true)==1);
    bool oversized=false;try{archer::installer_csw_packet(packet.data(),512,true);}catch(const std::runtime_error&){oversized=true;}assert(oversized);
    const std::array<std::uint8_t,18> sense{0x70,0,5,0,0,0,0,10,0,0,0,0,0x20,0,0,0,0,0};
    assert(archer::installer_fixed_sense(sense.data(),sense.size()));
    assert(!archer::installer_fixed_sense(sense.data(),13));
    bool not_csw=false;try{archer::installer_csw_packet(sense.data(),sense.size(),false);}catch(const std::runtime_error&){not_csw=true;}assert(not_csw);
    packet[13]=1;bool padding=false;try{archer::installer_csw_packet(packet.data(),512,true);}catch(const std::runtime_error&){padding=true;}assert(padding);
    bool short_read=false;try{archer::installer_csw_packet(packet.data(),12,true);}catch(const std::runtime_error&){short_read=true;}assert(short_read);
    csw[12]=2;bool phase=false;try{archer::installer_csw(csw,true);}catch(const std::runtime_error&){phase=true;}assert(phase);
    csw[12]=0;
    csw[4]=0;bool caught=false;try{archer::installer_csw(csw,true);}catch(const std::runtime_error&){caught=true;}assert(caught);
    std::cout<<"Installer eject packet encoding and status correlation: OK\n";
}
