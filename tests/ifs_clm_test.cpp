#include "archer/ifs_clm.hpp"
#include <map>
#include <cassert>
struct Bus:archer::RegisterBus{
 std::map<unsigned,unsigned> r{{0x10398,0},{0x20c00,0x100},{0x20c28,0xabc},{0x20c2c,1},{0x20c30,2},{0x20c34,3},{0x20c38,4}};
 unsigned writes=0,fail=0;
 unsigned read(unsigned a,unsigned)override{return r[a];}
 void write(unsigned a,unsigned,unsigned v)override{r[a]=v;if(++writes==fail)throw std::runtime_error("write failed after mutation");}
 void pause()override{}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){
 // Any setup or trigger write may fail after mutation: restore every control.
 for(unsigned fail=0;fail<=11;++fail){Bus b;auto original=b.r;b.fail=fail;
  auto run=[&]{archer::with_ifs_clm_window(b,[&]{
   assert((b.r[0x20c28]>>16)==62500);assert(((b.r[0x20c28]>>14)&3)==3);
   assert(b.r[0x20c2c]==0x28000&&b.r[0x20c38]==0x808021);
   archer::trigger_ifs_clm(b);
  });};
  if(fail)rejects(run);else run();
  for(auto a:{0x20c00u,0x20c28u,0x20c2cu,0x20c30u,0x20c34u,0x20c38u})assert(b.r[a]==original[a]);
 }
 Bus active;active.r[0x10398]=1;rejects([&]{archer::with_ifs_clm_window(active,[]{});});assert(!active.writes);
 Bus report;rejects([&]{(void)archer::read_ifs_clm(report);});
 report.r[0x20eec]=0x10007;report.r[0x20ecc]=0x20003;report.r[0x20ed0]=0x40005;report.r[0x20ed4]=0x60007;
 const auto r=archer::read_ifs_clm(report);assert(r.done&&r.total==7&&r.tx==3&&r.edcca==2&&r.cck_cca==5&&r.ofdm_cca==4&&r.cck_fa==7&&r.ofdm_fa==6);
}
