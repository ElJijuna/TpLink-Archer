#include "archer/rf_power.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0xfc,0x71},{0xf0,0x1000},{0x3f0,0x2200000},{0x74,0x901004fd},{0,0xc29e},{0xcc,0x29},{0x18,0xabc00000}};
    std::map<unsigned,unsigned> x{{0xe0,0},{0x90,0x90},{0x80,0xc6},{0x81,0xc6},{0x2d,0x30},{0x2e,0x30},{0xe1,0x52},{0xa1,0}};
    unsigned writes=0,fail_at=0;
    unsigned read(unsigned a,unsigned)override{return r[a];}
    void write(unsigned a,unsigned,unsigned v)override{
        ++writes;if(writes==fail_at)throw std::runtime_error("injected write failure");
        if(a==0x270){unsigned addr=v&255,mask=(v>>16)&255;if(((v>>24)&3)==1)r[a]=0x1000000|(x[addr]<<8)|addr;
            else {assert((v&0x7f000000)==0);x[addr]=(x[addr]&~mask)|((v>>8)&mask);r[a]=v&0x7fffffff;}}
        else r[a]=v;
    }
    void pause()override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    Bus adda;adda.r[0x84]=0x80f30007;adda.r[0x244]=0xffffffff;adda.r[0x16]=0;
    archer::enable_adda_power(adda);
    assert(adda.r[0x84]==0x80c30007 && adda.r[0x16]==0xc && adda.r[0x244]==0xefffffff);
    Bus wrong;wrong.r[0xf0]=0;rejects([&]{archer::enable_adda_power(wrong);});assert(wrong.writes==0);
    Bus b;auto original=b.x;bool ran=false;
    archer::with_rf_power_window(b,[&]{ran=true;assert(b.x[0xe0]==3&&b.x[0x90]==0x6f&&b.x[0x80]==0xeb&&b.x[0x81]==0xeb);assert((b.r[0]&0xc100)==0x100);});
    assert(ran&&b.x==original&&b.r[0]==0xc29e&&b.r[0xcc]==0x29&&b.r[0x18]==0xabc00000);
    Bus failure;rejects([&]{archer::with_rf_power_window(failure,[]{throw std::runtime_error("RF timeout");});});assert(failure.x==original&&failure.r[0]==0xc29e);
    Bus partial;partial.fail_at=20;rejects([&]{archer::with_rf_power_window(partial,[]{assert(false);});});assert(partial.x==original);
    Bus invalid;rejects([&]{archer::write_xtal_power_masked(invalid,0xfd,1,255);});assert(invalid.writes==0);
    std::cout<<"RF power sequence, bounded fields and restoration OK\n";
}
