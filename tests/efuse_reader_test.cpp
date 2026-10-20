#include "archer/efuse_reader.hpp"
#include <map>
#include <cassert>
#include <iostream>
struct Bus: archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0xfc,0x371},{0xf0,0x1000},{0x3f0,0x2200000},
        {0,0x39e},{0xcc,0x29},{0xa4,0x88300003},{0x68,0x80406804},{0x30,0}};
    bool timeout=false, fail_data=false, fail_burst_restore=false;
    unsigned commands=0, pauses=0;
    unsigned read(unsigned a,unsigned) override {
        if(a==0x34) { if(fail_data) throw std::runtime_error("Injected data read failure"); return r[0x30]&0xffff; }
        return r[a];
    }
    void write(unsigned a,unsigned,unsigned v) override {
        if(a==0xa4 && !(v&(1u<<19)) && fail_burst_restore) {
            fail_burst_restore=false; throw std::runtime_error("Injected restore failure");
        }
        r[a]=v;
        if(a==0x30) {
            assert((v&0xffff0000)==0); // Never write an OTP programming mode.
            ++commands; if(!timeout) r[a]|=1u<<29;
        }
    }
    void pause() override { ++pauses; }
    void restored() { assert(r[0]==0x39e && r[0xcc]==0x29 && r[0x68]==0x80406804 && r[0xa4]==0x88300003); }
};
template<class F> void rejects(F f) {bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main() {
    Bus success;auto maps=archer::read_ddv_efuse(success);success.restored();
    assert(maps.physical.size()==0x1300 && maps.phycap.size()==0x38);
    assert(success.commands==0x1300/4+0x38/4);
    assert(maps.phycap[0]==0 && maps.phycap[1]==0x17);
    Bus sec;const auto security=archer::read_ddv_efuse(sec,archer::EfuseProfile::firmware_security);sec.restored();
    assert(sec.commands==1 && security.physical.empty() && security.phycap.empty());
    assert((security.security==std::vector<std::uint8_t>{0x80,0x15,0,0}));
    Bus sec_timeout;sec_timeout.timeout=true;rejects([&]{archer::read_ddv_efuse(sec_timeout,archer::EfuseProfile::firmware_security);});sec_timeout.restored();assert(sec_timeout.commands==1);
    Bus bad_profile;rejects([&]{archer::read_ddv_efuse(bad_profile,static_cast<archer::EfuseProfile>(9));});assert(!bad_profile.commands);
    Bus timeout;timeout.timeout=true;rejects([&]{archer::read_ddv_efuse(timeout);});timeout.restored();assert(timeout.commands==1);
    Bus failed;failed.fail_data=true;rejects([&]{archer::read_ddv_efuse(failed);});failed.restored();
    Bus cleanup;cleanup.fail_data=true;cleanup.fail_burst_restore=true;
    rejects([&]{archer::read_ddv_efuse(cleanup);});
    assert(cleanup.r[0]==0x39e && cleanup.r[0xcc]==0x29 && cleanup.r[0x68]==0x80406804);
    Bus invalid;invalid.r[0xf0]=0;rejects([&]{archer::read_ddv_efuse(invalid);});assert(!invalid.commands);
    std::cout<<"Efuse read-only commands, bounds and restoration: OK\n";
}
