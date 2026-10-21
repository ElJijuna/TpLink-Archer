#include "archer/cmac_init.hpp"
#include <cassert>
#include <map>
#include <iostream>
struct Bus:archer::RegisterBus {
 std::map<unsigned,unsigned> regs{{0xfc,0x71},{0xf0,0x1000},{0xe000,0x40000007},{0x10000,0xf00001ff},{0x10004,0x400001ff},{0x9100,3}};
 unsigned writes=0,delay=0;bool cam_stuck=false,ba_stuck=false;
 unsigned read(unsigned a,unsigned w)override{auto v=regs[a];return v&(w==1?255u:w==2?65535u:0xffffffff);}
 void write(unsigned a,unsigned w,unsigned v)override{++writes;auto m=w==1?255u:w==2?65535u:0xffffffff;regs[a]=(regs[a]&~m)|(v&m);if(a==archer::cmac_detail::R_BE_ADDR_CAM_CTRL&&!cam_stuck)regs[a]&=~archer::cmac_detail::B_BE_ADDR_CAM_CLR;if(a==archer::cmac_detail::R_BE_RESPBA_CAM_CTRL&&!ba_stuck)regs[a]&=~3u;}
 void pause()override{assert(false);}
};
template<class F>void rejects(F f){bool yes=false;try{f();}catch(const std::exception&){yes=true;}assert(yes);}
int main(){using namespace archer;using namespace archer::cmac_detail;CmacInitInputs in;in.exclusive_initialization_owner=true;
 const auto plan=usb2_scc_cmac0_plan(in); unsigned polls=0; for(const auto& op:plan){assert((op.value&~op.mask)==0);assert(op.width==1||op.width==2||op.width==4);if(op.width<4)assert((op.mask>>(op.width*8))==0);polls+=op.kind==CmacOpKind::poll;}assert(polls==2);
 Bus b;initialize_usb2_scc_cmac0(b,in,[]{});
 assert(((b.regs[R_BE_ADDR_CAM_CTRL]&B_BE_ADDR_CAM_RANGE_MASK)>>16)==127);
 assert(b.regs[R_BE_DLK_PROTECT_CTL]==0x20f2);
 assert((b.regs[R_BE_RX_FLTR_OPT]&B_BE_RX_MPDU_MAX_LEN_MASK)==(23u<<16));
 assert((b.regs[R_BE_RESP_CSI_RESERVED_PAGE]&B_BE_CSI_RESERVED_START_PAGE_MASK)==2060);
 assert(((b.regs[R_BE_RESP_CSI_RESERVED_PAGE]&B_BE_CSI_RESERVED_PAGE_NUM_MASK)>>16)==57);
 assert(b.regs[R_BE_MGNT_FLTR]==65535 && b.regs[R_BE_CTRL_FLTR]==65535 && b.regs[R_BE_DATA_FLTR]==65535);
 assert((b.regs[R_BE_RX_CTRL_1]&B_BE_RXDMA_TXRPT_QUEUE_ID_SW_MASK)==(10u<<25));
 auto bad=in;bad.ple_free_pages=1;Bus no;rejects([&]{initialize_usb2_scc_cmac0(no,bad,[]{});});assert(!no.writes);
 Bus missing;missing.regs[0xe000]=0;rejects([&]{initialize_usb2_scc_cmac0(missing,in,[]{});});assert(!missing.writes);
 Bus cam;cam.cam_stuck=true;unsigned delay=0;rejects([&]{initialize_usb2_scc_cmac0(cam,in,[&]{++delay;});});assert(delay==2000 && !cam.regs.count(R_BE_RX_FLTR_OPT));
 Bus ba;ba.ba_stuck=true;delay=0;rejects([&]{initialize_usb2_scc_cmac0(ba,in,[&]{++delay;});});assert(delay==1000 && !ba.regs.count(R_BE_DLK_PROTECT_CTL));
 std::cout<<"CMAC0 complete USB2 SCC source-flow tests passed\n";
}
