#include "archer/xtal_identity.hpp"
#include <cassert>
#include <iostream>
#include <array>
#include <fstream>
struct Bus:archer::RegisterBus {
    unsigned control=0,writes=0,pauses=0;
    bool timeout=false,mismatch=false;
    std::array<unsigned char,3> identity{2,0x22,0x89};
    unsigned read(unsigned a,unsigned) override {
        if(a==0xfc)return 0x371;
        if(a==0x3f0)return 0x2200000;
        if(a==0xf0)return 0x1000;
        return control;
    }
    void write(unsigned a,unsigned,unsigned v) override {
        assert(a==0x270 && (v&0xffffff00)==0x81000000);
        ++writes;control=v;
        if(!timeout)control=(v&~(1u<<31))|((v&255)==0x41?unsigned(identity[0])<<8:(v&255)==0xfd?unsigned(identity[1])<<8:unsigned(identity[2])<<8);
        if(mismatch)control^=1;
    }
    void pause() override {++pauses;}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    Bus ok;auto id=archer::read_chip_identity(ok);assert(id.revision==1 && id.analog_revision==2 && id.analog_id==0x8922 && ok.writes==3);
    Bus physical;
    std::ifstream fixture("tests/fixtures/xtal-identity.bin",std::ios::binary);
    fixture.read(reinterpret_cast<char*>(physical.identity.data()),physical.identity.size());
    assert(fixture.gcount()==3);
    const auto actual=archer::read_chip_identity(physical);
    assert(actual.revision==1 && actual.analog_revision==1 && actual.analog_id==0x6933);
    Bus busy;busy.control=0x80000000;rejects([&]{archer::read_chip_identity(busy);});assert(!busy.writes);
    Bus invalid;rejects([&]{archer::read_xtal_identity_byte(invalid,0);});assert(!invalid.writes);
    Bus timeout;timeout.timeout=true;rejects([&]{archer::read_chip_identity(timeout);});assert(timeout.pauses==6 && timeout.writes==1);
    Bus wrong;wrong.mismatch=true;rejects([&]{archer::read_chip_identity(wrong);});
    std::cout<<"XTAL-SI read encoding, bounds and identity matching: OK\n";
}
