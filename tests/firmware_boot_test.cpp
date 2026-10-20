#include "archer/firmware_boot.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0xfc,0x371},{0xf0,0x1000},{0x3f0,0x2200000},{0x74,0x901004fd},
        {0x7880,3},{0xb000,0x430},{0xb010,0x80000000},{0x8420,0x1000},{0x84,0},{0x842c,0},{0x8d00,3},{0x9100,3},{0x904c,0x100},{0x1e0,0x0c000000}};
    unsigned writes=0,pauses=0;bool ready=true;
    unsigned read(unsigned a,unsigned)override{return r[a];}
    void write(unsigned a,unsigned,unsigned v)override{++writes;r[a]=v;if(a==0x88&&v==2&&ready&&r[0x1e0]==0xa20)r[0x1e0]|=2;}
    void pause()override{++pauses;}
};
std::vector<unsigned char> frame(unsigned type){std::vector<unsigned char> f(32);archer::channel_put32(f.data(),type<<24|8);return f;}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    const std::vector<archer::BootImage> images(2,archer::BootImage{{frame(13),frame(14)}});
    Bus ok;unsigned sent=0;std::vector<std::string> phases;
    archer::boot_firmware(ok,images,[&](const auto& f){++sent;if((f[3]&63)==13)ok.r[0x1e0]|=1;else if(sent==2)ok.r[0x1e0]&=~0x200u;else ok.r[0x1e0]=0x0c000003;},[&](const char* p){phases.emplace_back(p);});
    assert(sent==4&&phases.back()=="firmware_ready");assert(ok.r[0x36808]==0x76543210);
    Bus blocked;blocked.r[0x8d00]=0;rejects([&]{archer::boot_firmware(blocked,images,[](const auto&){assert(false);},[](const char*){});});assert(blocked.writes==0);
    Bus inaccessible;inaccessible.r[0x888]=0xdeadbeef;rejects([&]{archer::boot_firmware(inaccessible,images,[](const auto&){assert(false);},[](const char*){});});assert(inaccessible.writes==0);
    Bus timeout;timeout.ready=false;rejects([&]{archer::boot_firmware(timeout,images,[](const auto&){assert(false);},[](const char*){});});assert(timeout.pauses==101);
    Bus error;unsigned attempts=0;rejects([&]{archer::boot_firmware(error,images,[&](const auto&){++attempts;throw std::runtime_error("disconnect");},[](const char*){});});assert(attempts==1);
    Bus security;attempts=0;rejects([&]{archer::boot_firmware(security,images,[&](const auto&){++attempts;security.r[0x1e0]=5u<<26;},[](const char*){});});assert(attempts==1);
    Bus no_dcpu;no_dcpu.r[0x888]=0xdeadbeef;unsigned no_dcpu_sent=0;archer::boot_firmware(no_dcpu,images,[&](const auto& f){++no_dcpu_sent;if((f[3]&63)==13)no_dcpu.r[0x1e0]|=1;else if(no_dcpu_sent==2)no_dcpu.r[0x1e0]&=~0x200u;else no_dcpu.r[0x1e0]=0x0c000003;},[](const char*){},true);assert(no_dcpu.r[0x888]==0xdeadbeef&&no_dcpu_sent==4);
    Bus malformed;rejects([&]{archer::boot_firmware(malformed,{},[](const auto&){assert(false);},[](const char*){});});assert(malformed.writes==0);
    std::cout<<"Firmware boot ordering, timeout, ROM rejection and no-retry behavior OK\n";
}
