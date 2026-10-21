#pragma once
#include "archer/register_mailbox.hpp"
#include <vector>
namespace archer {
// Source-bound RTL8922A CV1 CMAC0, USB2 SCC; no PCIe/DBCC/CAV branches.
namespace cmac_detail {
inline constexpr unsigned ADDR_CAM_SERCH_RANGE=0x7f;
inline constexpr unsigned AMPDU_MAX_TIME=0x9e;
inline constexpr unsigned BCN_IFS_25US=0x19;
inline constexpr unsigned B_BE_ACK_BA_EHT_LEG_PUNC_CHK_EDCCA=0x2;
inline constexpr unsigned B_BE_ACK_BA_RESP_HE_CHK_EDCCA=0x2;
inline constexpr unsigned B_BE_ACK_BA_RESP_LEGACY_CHK_EDCCA=0x2;
inline constexpr unsigned B_BE_ADDR_CAM_CLR=0x100;
inline constexpr unsigned B_BE_ADDR_CAM_EN=0x1;
inline constexpr unsigned B_BE_ADDR_CAM_RANGE_MASK=0xff0000;
inline constexpr unsigned B_BE_AMPDU_MAX_TIME_MASK=0xff000000;
inline constexpr unsigned B_BE_BACAM_RST_MASK=0x3;
inline constexpr unsigned B_BE_BCNQ_AIFS_MASK=0xff0000;
inline constexpr unsigned B_BE_BCNQ_CW_MASK=0xff000000;
inline constexpr unsigned B_BE_BMC_NAV_PROTECT=0x4000000;
inline constexpr unsigned B_BE_BUSY_CHKSN=0x8000;
inline constexpr unsigned B_BE_CCK_CRC_CHK=0x1;
inline constexpr unsigned B_BE_CCK_SIG_CHK=0x2;
inline constexpr unsigned B_BE_CMAC_TX_MODE_0=0x1;
inline constexpr unsigned B_BE_CMAC_TX_MODE_1=0x2;
inline constexpr unsigned B_BE_CSI_RESERVED_PAGE_NUM_MASK=0xfff0000;
inline constexpr unsigned B_BE_CSI_RESERVED_START_PAGE_MASK=0xfff;
inline constexpr unsigned B_BE_EHT_HE_PPDU_2XLTF_ZLD_USTIMER_MASK=0x1f0000;
inline constexpr unsigned B_BE_EHT_HE_PPDU_4XLTF_ZLD_USTIMER_MASK=0x1f000000;
inline constexpr unsigned B_BE_EN_TX_FINISH_PRD_RESP=0x40000;
inline constexpr unsigned B_BE_FTM_RRSR_RATE_EN_MASK=0x1f000000;
inline constexpr unsigned B_BE_HE_CTN_CHK_BASIC_NAV=0x2000;
inline constexpr unsigned B_BE_HE_CTN_CHK_CCA_BITMAP=0x4;
inline constexpr unsigned B_BE_HE_CTN_CHK_CCA_P20=0x1;
inline constexpr unsigned B_BE_HE_CTN_CHK_EDCCA_BITMAP=0x8;
inline constexpr unsigned B_BE_HE_CTN_CHK_EDCCA_P20=0x2;
inline constexpr unsigned B_BE_HE_CTN_CHK_INTRA_NAV=0x4000;
inline constexpr unsigned B_BE_HE_CTN_CHK_NO_GNT_WL=0x1000;
inline constexpr unsigned B_BE_HE_CTN_CHK_TX_NAV=0x8000;
inline constexpr unsigned B_BE_HE_SIFS_CHK_EDCCA_BITMAP=0x8;
inline constexpr unsigned B_BE_HE_SIFS_CHK_EDCCA_P20=0x2;
inline constexpr unsigned B_BE_HE_SIFS_CHK_NO_GNT_WL=0x1000;
inline constexpr unsigned B_BE_HE_SIGB_CRC_CHK=0x40;
inline constexpr unsigned B_BE_LSIG_PARITY_CHK_EN=0x4;
inline constexpr unsigned B_BE_MACLBK_EN=0x1;
inline constexpr unsigned B_BE_MACLBK_PLCP_DLY_MASK=0x1ff00;
inline constexpr unsigned B_BE_NO_GNT_WL_EN=0x20;
inline constexpr unsigned B_BE_PHYINTF_EN=0x20;
inline constexpr unsigned B_BE_PLCP_SRC_EN=0x1;
inline constexpr unsigned B_BE_PLCP_SU_PSDU_LEN_SRC=0x100;
inline constexpr unsigned B_BE_PTCL_TRIGGER_SS_EN_0=0x4;
inline constexpr unsigned B_BE_PTCL_TRIGGER_SS_EN_1=0x8;
inline constexpr unsigned B_BE_PTCL_TRIGGER_SS_EN_UL=0x10;
inline constexpr unsigned B_BE_QOSNULL_UPD_MUEDCA_EN=0x8;
inline constexpr unsigned B_BE_RRSR_CCK_MASK=0xf;
inline constexpr unsigned B_BE_RRSR_HE_MASK=0xff000000;
inline constexpr unsigned B_BE_RRSR_HT_MASK=0xff00;
inline constexpr unsigned B_BE_RRSR_OFDM_MASK=0xff;
inline constexpr unsigned B_BE_RRSR_RATE_EN_MASK=0x1f00;
inline constexpr unsigned B_BE_RRSR_VHT_MASK=0xff0000;
inline constexpr unsigned B_BE_RSC_MASK=0xc0;
inline constexpr unsigned B_BE_RXDMA_F2PCMDRPT_QUEUE_ID_SW_MASK=0xfc0000;
inline constexpr unsigned B_BE_RXDMA_TXRPT_QUEUE_ID_SW_MASK=0x7e000000;
inline constexpr unsigned B_BE_RXTRIG_FCSCHK_EN=0x100000;
inline constexpr unsigned B_BE_RX_DLK_CCA_TIME_MASK=0xff00;
inline constexpr unsigned B_BE_RX_DLK_DATA_TIME_MASK=0xf0;
inline constexpr unsigned B_BE_RX_DLK_RST_EN=0x2;
inline constexpr unsigned B_BE_RX_MPDU_MAX_LEN_MASK=0x3f0000;
inline constexpr unsigned B_BE_SIGA_CRC_CHK=0x8;
inline constexpr unsigned B_BE_SR_CTRL_PLCP_EN=0x2;
inline constexpr unsigned B_BE_SR_EN=0x1;
inline constexpr unsigned B_BE_TB_CHK_BASIC_NAV=0x2000;
inline constexpr unsigned B_BE_TB_CHK_EDCCA_BITMAP=0x8;
inline constexpr unsigned B_BE_TB_CHK_NO_GNT_WL=0x1000;
inline constexpr unsigned B_BE_TXSB_160M_MASK=0xf000;
inline constexpr unsigned B_BE_TXSB_20M_MASK=0xf;
inline constexpr unsigned B_BE_TXSB_40M_MASK=0xf0;
inline constexpr unsigned B_BE_TXSB_80M_MASK=0xf00;
inline constexpr unsigned B_BE_VHT_MU_SIGB_CRC_CHK=0x20;
inline constexpr unsigned B_BE_VHT_SU_SIGB_CRC_CHK=0x10;
inline constexpr unsigned B_BE_WMAC_MBA_DUR_FORCE=0x10000;
inline constexpr unsigned B_BE_WMAC_NAV_UPPER_EN=0x4000000;
inline constexpr unsigned B_BE_WMAC_NAV_UPPER_MASK=0xff00;
inline constexpr unsigned B_BE_WMAC_PLCP_UP_NAV_EN=0x20000;
inline constexpr unsigned B_BE_WMAC_RESP_DCM_EN=0x100000;
inline constexpr unsigned B_BE_WMAC_RESP_DOPPLEB_BE_EN=0x200000;
inline constexpr unsigned B_BE_WMAC_RESP_REF_RATE_MASK=0xfff;
inline constexpr unsigned B_BE_WMAC_RESP_REF_RATE_SEL=0x1000;
inline constexpr unsigned B_BE_WMAC_SPEC_SIFS_CCK_MASK=0xff;
inline constexpr unsigned B_BE_WMAC_SPEC_SIFS_OFDM_MASK=0xff00;
inline constexpr unsigned B_BE_WMAC_TF_UP_NAV_EN=0x10000;
inline constexpr unsigned DEFAULT_BE_RX_FLTR=0xf0040be;
inline constexpr unsigned NAV_25MS=0xc4;
inline constexpr unsigned RX_FLTR_FRAME_ACCEPT_BE=0xffff;
inline constexpr unsigned R_BE_ADDR_CAM_CTRL=0x11434;
inline constexpr unsigned R_BE_AMPDU_AGG_LIMIT=0x10810;
inline constexpr unsigned R_BE_BSSID_SRC_CTRL=0x1144b;
inline constexpr unsigned R_BE_CCA_CFG_0=0x10340;
inline constexpr unsigned R_BE_CMAC_FUNC_EN=0x10000;
inline constexpr unsigned R_BE_CTRL_FLTR=0x11424;
inline constexpr unsigned R_BE_DATA_FLTR=0x1142c;
inline constexpr unsigned R_BE_DLK_PROTECT_CTL=0x11402;
inline constexpr unsigned R_BE_EDCA_BCNQ_PARAM=0x10324;
inline constexpr unsigned R_BE_HE_CTN_CHK_CCA_NAV=0x103c4;
inline constexpr unsigned R_BE_HE_SIFS_CHK_CCA_NAV=0x103b4;
inline constexpr unsigned R_BE_MAC_LOOPBACK=0x11020;
inline constexpr unsigned R_BE_MGNT_FLTR=0x11428;
inline constexpr unsigned R_BE_MISC_1=0x1037c;
inline constexpr unsigned R_BE_PLCP_HDR_FLTR=0x11404;
inline constexpr unsigned R_BE_PTCL_COMMON_SETTING_0=0x10800;
inline constexpr unsigned R_BE_PTCL_RRSR0=0x1008c;
inline constexpr unsigned R_BE_PTCL_RRSR1=0x10090;
inline constexpr unsigned R_BE_RCR=0x11400;
inline constexpr unsigned R_BE_RESPBA_CAM_CTRL=0x1143c;
inline constexpr unsigned R_BE_RESP_CSI_RESERVED_PAGE=0x11810;
inline constexpr unsigned R_BE_RXTRIG_TEST_USER_2=0x110b0;
inline constexpr unsigned R_BE_RX_CTRL_1=0x10c0c;
inline constexpr unsigned R_BE_RX_FLTR_OPT=0x11420;
inline constexpr unsigned R_BE_RX_PLCP_EXT_OPTION_1=0x11514;
inline constexpr unsigned R_BE_RX_SR_CTRL=0x1144a;
inline constexpr unsigned R_BE_SPECIAL_TX_SETTING=0x10820;
inline constexpr unsigned R_BE_TB_CHK_CCA_NAV=0x103ac;
inline constexpr unsigned R_BE_TB_PPDU_CTRL=0x1080c;
inline constexpr unsigned R_BE_TRXPTCL_RESP_0=0x11004;
inline constexpr unsigned R_BE_TRXPTCL_RESP_1=0x11008;
inline constexpr unsigned R_BE_TX_SUB_BAND_VALUE=0x10088;
inline constexpr unsigned R_BE_WMAC_ACK_BA_RESP_EHT_LEG_PUNC=0x11208;
inline constexpr unsigned R_BE_WMAC_ACK_BA_RESP_HE=0x11204;
inline constexpr unsigned R_BE_WMAC_ACK_BA_RESP_LEGACY=0x11200;
inline constexpr unsigned R_BE_WMAC_NAV_CTL=0x11080;
inline constexpr unsigned R_BE_WMTX_TCR_BE_4=0x10e2c;
inline constexpr unsigned S_BE_BACAM_RST_ALL=0x2;
inline constexpr unsigned S_BE_MACLBK_PLCP_DLY_DEF=0x28;
inline constexpr unsigned S_BE_TXSB_160M_1=0x1;
inline constexpr unsigned S_BE_TXSB_20M_8=0x8;
inline constexpr unsigned S_BE_TXSB_40M_4=0x4;
inline constexpr unsigned S_BE_TXSB_80M_2=0x2;
inline constexpr unsigned TRXCFG_RMAC_CCA_TO=0x20;
inline constexpr unsigned TRXCFG_RMAC_DATA_TO=0xf;
inline constexpr unsigned WLCPU_RXCH2_QID=0xa;
inline constexpr unsigned WMAC_SPEC_SIFS_CCK=0xa;
inline constexpr unsigned WMAC_SPEC_SIFS_OFDM_1115E=0x11;
}

struct CmacInitInputs {
 bool exclusive_initialization_owner=false;
 unsigned ple_free_pages=2059, ple_page_bytes=256, c0_rx_min_pages=463;
 unsigned mpdu_reserved_pages=1, csi_reserved_pages=56;
};
enum class CmacOpKind { write, poll };
struct CmacInitOp { CmacOpKind kind; unsigned address,width,mask,value,timeout_us=0; };
inline std::vector<CmacInitOp> usb2_scc_cmac0_plan(const CmacInitInputs& in) {
 using namespace cmac_detail;
 if(!in.exclusive_initialization_owner || in.ple_free_pages!=2059 || in.ple_page_bytes!=256 || in.c0_rx_min_pages!=463 || in.mpdu_reserved_pages!=1 || in.csi_reserved_pages!=56)
  throw std::runtime_error("CMAC0 requires exclusive RTL8922A CV1 USB2 SCC metadata");
 std::vector<CmacInitOp> p;
 auto w=[&](unsigned a,unsigned width,unsigned m,unsigned v){p.push_back({CmacOpKind::write,a,width,m,v});};
 auto set=[&](unsigned a,unsigned m){w(a,4,m,m);};
 auto clr=[&](unsigned a,unsigned m){w(a,4,m,0);};
 auto field=[&](unsigned a,unsigned m,unsigned v){unsigned shift=0;while(!(m&(1u<<shift)))++shift;w(a,4,m,(v<<shift)&m);};
 auto full=[&](unsigned a,unsigned v){w(a,4,0xffffffff,v);};
 auto poll=[&](unsigned a,unsigned width,unsigned m,unsigned v,unsigned t){p.push_back({CmacOpKind::poll,a,width,m,v,t});};
 set(R_BE_MISC_1,B_BE_EN_TX_FINISH_PRD_RESP);
 full(R_BE_HE_CTN_CHK_CCA_NAV,B_BE_HE_CTN_CHK_CCA_P20|B_BE_HE_CTN_CHK_EDCCA_P20|B_BE_HE_CTN_CHK_CCA_BITMAP|B_BE_HE_CTN_CHK_EDCCA_BITMAP|B_BE_HE_CTN_CHK_NO_GNT_WL|B_BE_HE_CTN_CHK_BASIC_NAV|B_BE_HE_CTN_CHK_INTRA_NAV|B_BE_HE_CTN_CHK_TX_NAV);
 full(R_BE_HE_SIFS_CHK_CCA_NAV,B_BE_HE_SIFS_CHK_EDCCA_P20|B_BE_HE_SIFS_CHK_EDCCA_BITMAP|B_BE_HE_SIFS_CHK_NO_GNT_WL);
 full(R_BE_TB_CHK_CCA_NAV,B_BE_TB_CHK_EDCCA_BITMAP|B_BE_TB_CHK_NO_GNT_WL|B_BE_TB_CHK_BASIC_NAV);
 clr(R_BE_CCA_CFG_0,B_BE_NO_GNT_WL_EN);
 field(R_BE_EDCA_BCNQ_PARAM,B_BE_BCNQ_CW_MASK,0x32);field(R_BE_EDCA_BCNQ_PARAM,B_BE_BCNQ_AIFS_MASK,BCN_IFS_25US);
 w(R_BE_ADDR_CAM_CTRL,4,B_BE_ADDR_CAM_RANGE_MASK|B_BE_ADDR_CAM_EN|B_BE_ADDR_CAM_CLR,(ADDR_CAM_SERCH_RANGE<<16)|B_BE_ADDR_CAM_EN|B_BE_ADDR_CAM_CLR);
 poll(R_BE_ADDR_CAM_CTRL,2,B_BE_ADDR_CAM_CLR,0,2000);
 full(R_BE_MGNT_FLTR,RX_FLTR_FRAME_ACCEPT_BE);full(R_BE_CTRL_FLTR,RX_FLTR_FRAME_ACCEPT_BE);full(R_BE_DATA_FLTR,RX_FLTR_FRAME_ACCEPT_BE);
 full(R_BE_RX_FLTR_OPT,DEFAULT_BE_RX_FLTR);
 w(R_BE_PLCP_HDR_FLTR,2,0xffff,B_BE_HE_SIGB_CRC_CHK|B_BE_VHT_MU_SIGB_CRC_CHK|B_BE_VHT_SU_SIGB_CRC_CHK|B_BE_SIGA_CRC_CHK|B_BE_LSIG_PARITY_CHK_EN|B_BE_CCK_SIG_CHK|B_BE_CCK_CRC_CHK);
 w(R_BE_WMAC_NAV_CTL,4,B_BE_WMAC_PLCP_UP_NAV_EN|B_BE_WMAC_TF_UP_NAV_EN|B_BE_WMAC_NAV_UPPER_EN|B_BE_WMAC_NAV_UPPER_MASK,B_BE_WMAC_TF_UP_NAV_EN|B_BE_WMAC_NAV_UPPER_EN|(NAV_25MS<<8));
 clr(R_BE_SPECIAL_TX_SETTING,B_BE_BMC_NAV_PROTECT);set(R_BE_TRXPTCL_RESP_0,B_BE_WMAC_MBA_DUR_FORCE);
 w(R_BE_RX_SR_CTRL,1,B_BE_SR_EN|B_BE_SR_CTRL_PLCP_EN,0);w(R_BE_BSSID_SRC_CTRL,1,B_BE_PLCP_SRC_EN,B_BE_PLCP_SRC_EN);
 clr(R_BE_TB_PPDU_CTRL,B_BE_QOSNULL_UPD_MUEDCA_EN);
 field(R_BE_WMTX_TCR_BE_4,B_BE_EHT_HE_PPDU_4XLTF_ZLD_USTIMER_MASK,0x12);field(R_BE_WMTX_TCR_BE_4,B_BE_EHT_HE_PPDU_2XLTF_ZLD_USTIMER_MASK,0xe);
 w(R_BE_MAC_LOOPBACK,4,B_BE_MACLBK_PLCP_DLY_MASK|B_BE_MACLBK_EN,S_BE_MACLBK_PLCP_DLY_DEF<<8);
 set(R_BE_CMAC_FUNC_EN,B_BE_PHYINTF_EN);
 w(R_BE_TRXPTCL_RESP_0,4,B_BE_WMAC_SPEC_SIFS_CCK_MASK|B_BE_WMAC_SPEC_SIFS_OFDM_MASK,WMAC_SPEC_SIFS_CCK|(WMAC_SPEC_SIFS_OFDM_1115E<<8));
 clr(R_BE_WMAC_ACK_BA_RESP_LEGACY,B_BE_ACK_BA_RESP_LEGACY_CHK_EDCCA);clr(R_BE_WMAC_ACK_BA_RESP_HE,B_BE_ACK_BA_RESP_HE_CHK_EDCCA);clr(R_BE_WMAC_ACK_BA_RESP_EHT_LEG_PUNC,B_BE_ACK_BA_EHT_LEG_PUNC_CHK_EDCCA);
 set(R_BE_RXTRIG_TEST_USER_2,B_BE_RXTRIG_FCSCHK_EN);
 clr(R_BE_TRXPTCL_RESP_1,~(B_BE_FTM_RRSR_RATE_EN_MASK|B_BE_WMAC_RESP_DOPPLEB_BE_EN|B_BE_WMAC_RESP_DCM_EN|B_BE_WMAC_RESP_REF_RATE_MASK));
 clr(R_BE_TRXPTCL_RESP_1,B_BE_WMAC_RESP_REF_RATE_SEL);
 clr(R_BE_PTCL_RRSR1,~(B_BE_RRSR_RATE_EN_MASK|B_BE_RRSR_CCK_MASK|B_BE_RSC_MASK));
 clr(R_BE_PTCL_RRSR0,~(B_BE_RRSR_OFDM_MASK|B_BE_RRSR_HT_MASK|B_BE_RRSR_VHT_MASK|B_BE_RRSR_HE_MASK));
 field(R_BE_RESPBA_CAM_CTRL,B_BE_BACAM_RST_MASK,S_BE_BACAM_RST_ALL);
 poll(R_BE_RESPBA_CAM_CTRL,4,B_BE_BACAM_RST_MASK,0,1000);
 w(R_BE_DLK_PROTECT_CTL,2,B_BE_RX_DLK_DATA_TIME_MASK|B_BE_RX_DLK_CCA_TIME_MASK|B_BE_RX_DLK_RST_EN,(TRXCFG_RMAC_DATA_TO<<4)|(TRXCFG_RMAC_CCA_TO<<8)|B_BE_RX_DLK_RST_EN);
 field(R_BE_RX_FLTR_OPT,B_BE_RX_MPDU_MAX_LEN_MASK,23);
 w(R_BE_PLCP_HDR_FLTR,1,B_BE_VHT_SU_SIGB_CRC_CHK,0);
 w(R_BE_RCR,2,B_BE_BUSY_CHKSN,B_BE_BUSY_CHKSN);w(R_BE_RX_PLCP_EXT_OPTION_1,2,B_BE_PLCP_SU_PSDU_LEN_SRC,B_BE_PLCP_SU_PSDU_LEN_SRC);
 field(R_BE_RESP_CSI_RESERVED_PAGE,B_BE_CSI_RESERVED_START_PAGE_MASK,in.ple_free_pages+in.mpdu_reserved_pages);
 field(R_BE_RESP_CSI_RESERVED_PAGE,B_BE_CSI_RESERVED_PAGE_NUM_MASK,in.csi_reserved_pages+1);
 w(R_BE_TX_SUB_BAND_VALUE,4,0xffff,S_BE_TXSB_20M_8|(S_BE_TXSB_40M_4<<4)|(S_BE_TXSB_80M_2<<8)|(S_BE_TXSB_160M_1<<12));
 w(R_BE_PTCL_COMMON_SETTING_0,1,B_BE_CMAC_TX_MODE_0|B_BE_CMAC_TX_MODE_1|B_BE_PTCL_TRIGGER_SS_EN_0|B_BE_PTCL_TRIGGER_SS_EN_1|B_BE_PTCL_TRIGGER_SS_EN_UL,B_BE_CMAC_TX_MODE_0|B_BE_CMAC_TX_MODE_1);
 field(R_BE_AMPDU_AGG_LIMIT,B_BE_AMPDU_MAX_TIME_MASK,AMPDU_MAX_TIME);
 field(R_BE_RX_CTRL_1,B_BE_RXDMA_TXRPT_QUEUE_ID_SW_MASK,WLCPU_RXCH2_QID);field(R_BE_RX_CTRL_1,B_BE_RXDMA_F2PCMDRPT_QUEUE_ID_SW_MASK,WLCPU_RXCH2_QID);
 return p;
}
inline void initialize_usb2_scc_cmac0(RegisterBus& bus,const CmacInitInputs& in,const std::function<void()>& delay_one_us) {
 const auto plan=usb2_scc_cmac0_plan(in);
 if(!delay_one_us)throw std::runtime_error("CMAC polling requires >=1us delay");
 auto read=[&](unsigned a,unsigned width){auto v=bus.read(a,width);if(v==0xdeadbeef||v==0xeaeaeaea)throw std::runtime_error("CMAC register inaccessible; reset required");return v;};
 if((read(0xfc,4)&255)!=0x71 || ((read(0xf0,4)>>12)&15)!=1 || (read(0xe000,4)&0x40000007)!=0x40000007 || (read(0x10000,4)&0xf00001ff)!=0xf00001ff || (read(0x10004,4)&0x400001ff)!=0x400001ff || (read(0x9100,4)&3)!=3)
  throw std::runtime_error("CMAC0 power/clock/shared-CMAC/DLE prerequisites failed before writes");
 for(const auto& op:plan){
  if(op.kind==CmacOpKind::poll){bool done=false;for(unsigned elapsed=0;elapsed<=op.timeout_us;++elapsed){if((read(op.address,op.width)&op.mask)==op.value){done=true;break;}if(elapsed<op.timeout_us)delay_one_us();}if(!done)throw std::runtime_error("CMAC CAM reset timeout; reset required");}
  else {const auto v=op.mask==0xffffffff?op.value:(read(op.address,op.width)&~op.mask)|op.value;bus.write(op.address,op.width,v);}
 }
}

}
