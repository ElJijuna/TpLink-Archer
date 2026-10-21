#pragma once
#include "archer/register_mailbox.hpp"
#include <vector>
namespace archer {
struct TrxPathWrite { unsigned address,mask,value; };
struct TrxPathInputs {
 bool scheduler_paused_and_reset_owned=false;
 unsigned phy_index=0;
 bool dbcc=false;
 unsigned tx_antenna_mask=3,rx_antenna_mask=3,tx_nss=2,rx_nss=2;
};
// Addresses are physical (BB logical + 0x20000); path-com controls are MAC.
inline std::vector<TrxPathWrite> rtl8922a_scc_ab_trx_path_plan(const TrxPathInputs& in) {
 if(!in.scheduler_paused_and_reset_owned || in.phy_index || in.dbcc ||
    in.tx_antenna_mask!=3 || in.rx_antenna_mask!=3 || in.tx_nss!=2 || in.rx_nss!=2)
  throw std::runtime_error("TRX path plan requires reset-owned PHY0 SCC AB antennas and two streams");
 return {
        // Source ctrl_trx_path iteration PHY0; SCC indexed BB writes share PHY0.
        {0x209a4,0xe0000,0x0}, // R_MAC_SEL, B_MAC_SEL=0
        {0x11a00,0xffffffff,0x21c86900}, // TX AB path-com MAC register
        {0x11a04,0xffffffff,0xe4e433}, // TX AB path-com MAC register
        {0x11a08,0xffffffff,0x39390cc9}, // TX AB path-com MAC register
        {0x11a0c,0xffffffff,0x4e433240}, // TX AB path-com MAC register
        {0x11a10,0xffffffff,0x90cc900e}, // TX AB path-com MAC register
        {0x11a14,0xffffffff,0x240393}, // TX AB path-com MAC register
        {0x11a18,0xffffffff,0x201c8600}, // TX AB path-com MAC register
        {0x26b54,0xf,0x0}, // R_ANT_CHBW, B_ANT_RX_SG0=0
        {0x26b54,0xf,0x3}, // R_ANT_CHBW, B_ANT_RX_SG0=3
        {0x26b50,0x3c000,0xc000}, // R_FC0INV_SBW, B_RX_1RCCA=3
        {0x20418,0x300,0x100}, // R_BRK_R, B_HTMCS_LMT=1
        {0x20418,0x600000,0x200000}, // R_BRK_R, B_VHTMCS_LMT=1
        {0x20480,0x3fc0,0x100}, // R_BRK_HE, B_N_USR_MAX=4
        {0x20480,0x1c000,0x4000}, // R_BRK_HE, B_NSS_MAX=1
        {0x20480,0x3800000,0x800000}, // R_BRK_HE, B_TB_NSS_MAX=1
        {0x20474,0x1c,0x4}, // R_BRK_EHT, B_RXEHT_NSS_MAX=1
        {0x20478,0x1c000,0x4000}, // R_BRK_RXEHT, B_RXEHTTB_NSS_MAX=1
        {0x20478,0xff000000,0x4000000}, // R_BRK_RXEHT, B_RXEHT_N_USER_MAX=4
        {0x2e60c,0x10000,0x0}, // R_TXPWR_RSTA, B_TXPWR_RSTA=0
        {0x2e60c,0x10000,0x10000}, // R_TXPWR_RSTA, B_TXPWR_RSTA=1
        {0x2e70c,0x10000,0x0}, // R_TXPWR_RSTB, B_TXPWR_RSTB=0
        {0x2e70c,0x10000,0x10000}, // R_TXPWR_RSTB, B_TXPWR_RSTB=1
        {0x20418,0x300,0x100}, // R_BRK_R, B_HTMCS_LMT=1
        {0x20418,0x600000,0x200000}, // R_BRK_R, B_VHTMCS_LMT=1
        {0x20480,0x3fc0,0x100}, // R_BRK_HE, B_N_USR_MAX=4
        {0x20480,0x1c000,0x4000}, // R_BRK_HE, B_NSS_MAX=1
        {0x20480,0x3800000,0x800000}, // R_BRK_HE, B_TB_NSS_MAX=1
        {0x20474,0x1c,0x4}, // R_BRK_EHT, B_RXEHT_NSS_MAX=1
        {0x20478,0x1c000,0x4000}, // R_BRK_RXEHT, B_RXEHTTB_NSS_MAX=1
        {0x20478,0xff000000,0x4000000}, // R_BRK_RXEHT, B_RXEHT_N_USER_MAX=4
        // Source ctrl_trx_path iteration PHY1; SCC indexed BB writes share PHY0.
        {0x209a4,0xe0000,0x0}, // R_MAC_SEL, B_MAC_SEL=0
        {0x26b54,0xf,0x0}, // R_ANT_CHBW, B_ANT_RX_SG0=0
        {0x26b54,0xf,0x3}, // R_ANT_CHBW, B_ANT_RX_SG0=3
        {0x26b50,0x3c000,0xc000}, // R_FC0INV_SBW, B_RX_1RCCA=3
        {0x20418,0x300,0x100}, // R_BRK_R, B_HTMCS_LMT=1
        {0x20418,0x600000,0x200000}, // R_BRK_R, B_VHTMCS_LMT=1
        {0x20480,0x3fc0,0x100}, // R_BRK_HE, B_N_USR_MAX=4
        {0x20480,0x1c000,0x4000}, // R_BRK_HE, B_NSS_MAX=1
        {0x20480,0x3800000,0x800000}, // R_BRK_HE, B_TB_NSS_MAX=1
        {0x20474,0x1c,0x4}, // R_BRK_EHT, B_RXEHT_NSS_MAX=1
        {0x20478,0x1c000,0x4000}, // R_BRK_RXEHT, B_RXEHTTB_NSS_MAX=1
        {0x20478,0xff000000,0x4000000}, // R_BRK_RXEHT, B_RXEHT_N_USER_MAX=4
        {0x2e60c,0x10000,0x0}, // R_TXPWR_RSTA, B_TXPWR_RSTA=0
        {0x2e60c,0x10000,0x10000}, // R_TXPWR_RSTA, B_TXPWR_RSTA=1
        {0x2e70c,0x10000,0x0}, // R_TXPWR_RSTB, B_TXPWR_RSTB=0
        {0x2e70c,0x10000,0x10000}, // R_TXPWR_RSTB, B_TXPWR_RSTB=1
        {0x20418,0x300,0x100}, // R_BRK_R, B_HTMCS_LMT=1
        {0x20418,0x600000,0x200000}, // R_BRK_R, B_VHTMCS_LMT=1
        {0x20480,0x3fc0,0x100}, // R_BRK_HE, B_N_USR_MAX=4
        {0x20480,0x1c000,0x4000}, // R_BRK_HE, B_NSS_MAX=1
        {0x20480,0x3800000,0x800000}, // R_BRK_HE, B_TB_NSS_MAX=1
        {0x20474,0x1c,0x4}, // R_BRK_EHT, B_RXEHT_NSS_MAX=1
        {0x20478,0x1c000,0x4000}, // R_BRK_RXEHT, B_RXEHTTB_NSS_MAX=1
        {0x20478,0xff000000,0x4000000}, // R_BRK_RXEHT, B_RXEHT_N_USER_MAX=4
    };
}
// Initial core.c SCC mode is MLO_DBCC_NOT_SUPPORT (1). RF count is in bits12+.
// The 2+0 2RF enum (0x2002) is not accepted by RTL8922A ctrl_mlo; source
// bb_sethw ignores that error and still completes only its first three writes.
enum class SccBbMode : unsigned { initial_no_mlo=1, two_plus_zero_one_rf=0x1002,
                                two_plus_zero_two_rf=0x2002 };
inline bool rtl8922a_ctrl_mlo_supported(SccBbMode mode) {
 return mode==SccBbMode::initial_no_mlo || mode==SccBbMode::two_plus_zero_one_rf;
}
inline std::vector<TrxPathWrite> rtl8922a_scc_bb_sethw_plan(SccBbMode mode=SccBbMode::initial_no_mlo,
                                                         unsigned channel0_bw_mhz=20,
                                                         unsigned channel1_bw_mhz=20) {
 if(channel0_bw_mhz!=20 || channel1_bw_mhz!=20 ||
    (mode!=SccBbMode::initial_no_mlo && mode!=SccBbMode::two_plus_zero_one_rf &&
     mode!=SccBbMode::two_plus_zero_two_rf))
  throw std::runtime_error("SCC BB hardware plan requires explicit supported enum and both source channel widths20MHz");
 std::vector<TrxPathWrite> p{{0x2047c,2,0},{0x2147c,2,0},{0x11a40,0x10000,0}};
 if(!rtl8922a_ctrl_mlo_supported(mode))return p;
 p.push_back({0x26b48,1,0}); // R_DBCC, disable
 p.push_back({0x2703c,0x1000,0x1000}); // R_DBCC_FA
 p.push_back({0x22a5c,0xf8000000,0x70000000}); // AFEDAC0 A:14
 p.push_back({0x22a60,7,7});
 p.push_back({0x22b5c,0xf8000000,0x70000000}); // AFEDAC0 B:14
 p.push_back({0x22b60,7,7});
 auto emlsr=[&](unsigned value){p.push_back({0x20044,0x0ffff000,value<<12});};
 emlsr(0x6180);
 if(mode==SccBbMode::two_plus_zero_one_rf){emlsr(0xbbab);emlsr(0xaba9);emlsr(0xeba9);emlsr(0xeaa9);}
 else {emlsr(0x180);emlsr(0);}
 return p;
}
// The selected source callback returns before any writes for SCC.
inline std::vector<TrxPathWrite> rtl8922a_scc_post_set_channel_bb_plan(unsigned phy_index=0,bool dbcc=false) {
 if(phy_index || dbcc)throw std::runtime_error("Only PHY0 SCC post-channel callback is supported");
 return {};
}
inline void apply_rtl8922a_scc_ab_trx_path(RegisterBus& bus,const TrxPathInputs& in) {
 const auto plan=rtl8922a_scc_ab_trx_path_plan(in);
 for(const auto& op:plan){
  const auto old=op.mask==0xffffffff?0:bus.read(op.address,4);
  if(old==0xdeadbeef || old==0xeaeaeaea)throw std::runtime_error("TRX path register inaccessible; keep scheduler stopped and reset");
  bus.write(op.address,4,(old&~op.mask)|op.value);
 }
}
}
