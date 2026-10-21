#include "archer/channel_power.hpp"
#include <cassert>
#include <iostream>
template<class F> void rejects(F f){bool x=false;try{f();}catch(const std::exception&){x=true;}assert(x);}
int main(){
 archer::Channel1Context c;archer::Channel1BoardTrim t;
 assert(archer::channel1_board_trim_plan(c,t,0x6933,1,1).empty());
 t.pa_and_pad_programmed=true;t.pa_raw={0xab,0x12};t.pad_raw={0xcd,0x34};
 auto p=archer::channel1_board_trim_plan(c,t,0x6933,1,1);
 assert(p.size()==8&&p[0].value==11&&p[1].mask==0xf00&&p[1].value==10);
 assert(p[4].mask==0xf0&&p[4].value==13&&p[7].value==3);
 rejects([&]{archer::channel1_board_trim_plan(c,t,0x6933,2,1);});
 assert(archer::channel1_txpwr_unit_plan(c).empty());
 archer::Channel1PowerContext power;
 rejects([&]{archer::channel1_txpwr_reference_plan(c,power);});
 power.antenna_offset_bb=8;auto ref=archer::channel1_txpwr_reference_plan(c,power);
 assert(ref.size()==8&&ref[0].value==0&&ref[3].value==508);
 assert(ref[5].value==26u<<20&&ref[6].address==0x11a20);
 assert(!power.complete());power.antenna_offset_bb=-128;
 rejects([&]{archer::channel1_txpwr_reference_plan(c,power);});
 archer::WorldDiagnosticPowerProfile profile;
 rejects([&]{archer::validate_world_diagnostic_power(c,profile);});
 profile.scheduler_tx_paused=true;profile.fresh_unconfigured_host_state=true;
 archer::validate_world_diagnostic_power(c,profile);profile.antenna_regd_enabled=1;
 rejects([&]{archer::validate_world_diagnostic_power(c,profile);});
 std::cout<<"channel power tests passed\n";
}
