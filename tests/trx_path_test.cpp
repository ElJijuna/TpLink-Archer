#include "archer/trx_path.hpp"
#include <cassert>
#include <map>
#include <tuple>
#include <iostream>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> regs;
 std::vector<std::tuple<unsigned,unsigned,unsigned>> writes;
 unsigned read(unsigned a,unsigned)override{return regs.count(a)?regs[a]:0xa5a5a5a5;}
 void write(unsigned a,unsigned w,unsigned v)override{writes.emplace_back(a,w,v);regs[a]=v;}
 void pause()override{assert(false);}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){using namespace archer;
 TrxPathInputs in;in.scheduler_paused_and_reset_owned=true;
 const auto p=rtl8922a_scc_ab_trx_path_plan(in);assert(p.size()==55);
 unsigned mac_writes=0,rx_initial_zero=0;
 for(const auto& op:p){assert(op.mask && !(op.value&~op.mask));if(op.address<0x20000){++mac_writes;assert(op.address>=0x11a00 && op.address<=0x11a18 && op.mask==0xffffffff);}if(op.address==0x26b54&&!op.value)++rx_initial_zero;}
 assert(mac_writes==7 && rx_initial_zero==2);
 Bus b;apply_rtl8922a_scc_ab_trx_path(b,in);assert(b.writes.size()==55);
 assert(b.regs[0x11a00]==0x21c86900 && b.regs[0x11a18]==0x201c8600);
 assert((b.regs[0x26b54]&15)==3 && (b.regs[0x26b54]&~15u)==(0xa5a5a5a5&~15u));
 assert((b.regs[0x20418]&0x600300)==0x200100);
 assert((b.regs[0x20480]&0x381ffc0)==0x804100);
 for(unsigned i=32;i<55;++i)assert(p[i].address==p[i-24].address && p[i].mask==p[i-24].mask && p[i].value==p[i-24].value);
 auto bad=in;bad.dbcc=true;Bus no;rejects([&]{apply_rtl8922a_scc_ab_trx_path(no,bad);});assert(no.writes.empty());
 bad=in;bad.rx_nss=1;rejects([&]{rtl8922a_scc_ab_trx_path_plan(bad);});
 Bus missing;missing.regs[0x209a4]=0xdeadbeef;rejects([&]{apply_rtl8922a_scc_ab_trx_path(missing,in);});assert(missing.writes.empty());
 assert(rtl8922a_scc_post_set_channel_bb_plan().empty());rejects([&]{rtl8922a_scc_post_set_channel_bb_plan(0,true);});
 auto hw=rtl8922a_scc_bb_sethw_plan();assert(hw.size()==12);
 assert(hw[0].address==0x2047c && hw[1].address==0x2147c && hw[2].address==0x11a40);
 assert(hw[9].address==0x20044 && hw[9].value==0x6180000);
 assert(hw[10].value==0x180000 && !hw[11].value);
 for(const auto& op:hw)assert(!(op.value&~op.mask));
 auto one=rtl8922a_scc_bb_sethw_plan(SccBbMode::two_plus_zero_one_rf);assert(one.size()==14 && one.back().value==0xeaa9000);
 auto unsupported=rtl8922a_scc_bb_sethw_plan(SccBbMode::two_plus_zero_two_rf);assert(unsupported.size()==3 && !rtl8922a_ctrl_mlo_supported(SccBbMode::two_plus_zero_two_rf));
 rejects([&]{rtl8922a_scc_bb_sethw_plan(SccBbMode::initial_no_mlo,20,40);});
 std::cout<<"TRX AB two-stream SCC source-order tests passed\n";
}
