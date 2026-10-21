#include "archer/tx_rfk.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <iterator>
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){using namespace archer;
 RfkLogChunk iq{1,std::vector<std::uint8_t>(516)};iq.content[23]=iq.content[24]=1;
 assert(decode_iqk_report(iq).flags_clear());assert(decode_iqk_report(iq).matches({0,0,1},0));
 for(unsigned i=0;i<4;++i){auto bad=iq;bad.content[i]=1;assert(!decode_iqk_report(bad).flags_clear());}
 iq.content[4]=2;rejects([&]{decode_iqk_report(iq);});iq.content[4]=0;iq.content.pop_back();rejects([&]{decode_iqk_report(iq);});
 auto iq_legacy=iq;iq_legacy.content.resize(184);iq_legacy.content[23]=iq_legacy.content[24]=1;
 assert(decode_iqk_report(iq_legacy).flags_clear() && decode_iqk_report(iq_legacy).matches({0,0,1},0));
 std::ifstream iq_physical_file("tests/fixtures/iqk-stage39-usb.bin",std::ios::binary);assert(iq_physical_file.good());
 const std::vector<std::uint8_t> iq_physical{std::istreambuf_iterator<char>(iq_physical_file),{}};
 const auto iq_packets=decode_usb_rx_be(iq_physical);assert(iq_packets.size()==1);
 const auto iq_reports=decode_iqk_log_reports(iq_physical.data()+iq_packets[0].payload_offset,iq_packets[0].payload_bytes);
 assert(iq_reports.size()==1 && iq_reports[0].raw.size()==184);
 assert(iq_reports[0].flags_clear() && iq_reports[0].matches({0,0,1},0));
 RfkLogChunk dp{1,std::vector<std::uint8_t>(34)};dp.content[9]=dp.content[10]=1;dp.content[7]=dp.content[8]=1;
 assert(decode_dpk_report(dp).error_flags_clear() && !decode_dpk_report(dp).path_success());assert(decode_dpk_report(dp).matches({0,0,1}));
 for(unsigned i:{9,10,29,30,31,32}){auto bad=dp;bad.content[i]=(i<11?0:1);assert(!decode_dpk_report(bad).flags_clear());}
 std::ifstream dpk_physical_file("tests/fixtures/dpk-stage40-usb.bin",std::ios::binary);assert(dpk_physical_file.good());
 const std::vector<std::uint8_t> dpk_physical{std::istreambuf_iterator<char>(dpk_physical_file),{}};
 const auto dpk_packets=decode_usb_rx_be(dpk_physical);assert(dpk_packets.size()==1);
 const auto dpk_reports=decode_dpk_log_reports(dpk_physical.data()+dpk_packets[0].payload_offset,dpk_packets[0].payload_bytes);
 assert(dpk_reports.size()==1 && dpk_reports[0].version==13 && dpk_reports[0].error_flags_clear());
 assert((dpk_reports[0].path_ok==std::array<unsigned,2>{4,4}));
 assert(!dpk_reports[0].path_success() && !dpk_reports[0].flags_clear() && dpk_reports[0].matches({0,0,1}));
 RfkLogChunk tx{1,std::vector<std::uint8_t>(100)};tx.content[80]=1;tx.content[82]=7;tx.content[92]=0x34;tx.content[93]=0x12;
 assert((*decode_txgapk_report(tx).failcode)[0]==0x1234 && !decode_txgapk_report(tx).flags_clear());
 tx.content[92]=tx.content[93]=0;assert(decode_txgapk_report(tx).flags_clear());
 auto legacy=tx;legacy.content.resize(84);legacy.content[83]=255;
 assert(decode_txgapk_report(legacy).flags_clear());
 assert(!decode_txgapk_report(legacy).failcode && !decode_txgapk_report(legacy).stage && !decode_txgapk_report(legacy).dual_band_ok);
 legacy.content[80]=0;assert(!decode_txgapk_report(legacy).flags_clear());
 std::ifstream physical_file("tests/fixtures/txgapk-stage38-usb.bin",std::ios::binary);assert(physical_file.good());
 const std::vector<std::uint8_t> physical{std::istreambuf_iterator<char>(physical_file),{}};
 const auto packets=decode_usb_rx_be(physical);assert(packets.size()==1);
 const auto physical_reports=decode_txgapk_log_reports(physical.data()+packets[0].payload_offset,packets[0].payload_bytes);
 assert(physical_reports.size()==1 && physical_reports[0].raw.size()==84);
 assert(physical_reports[0].flags_clear() && !physical_reports[0].failcode);
 RfkLogChunk ts{1,std::vector<std::uint8_t>(56)};ts.content[0]=255;ts.content[16]=0x12;ts.content[32]=0x34;
 const auto tr=decode_tssi_report(ts);assert(tr.alignment_power[0]==-1 && tr.alignment_code[0]==0x1234);
 for(auto chunk:{dp,tx,ts}){chunk.content.pop_back();rejects([&]{if(chunk.content.size()==33)decode_dpk_report(chunk);else if(chunk.content.size()==99)decode_txgapk_report(chunk);else decode_tssi_report(chunk);});}
 rejects([&]{require_tx_rfk_prerequisites({true,true,false,true,true});});require_tx_rfk_prerequisites({true,true,true,true,true});
 TssiBoardData board{};board.rfe=1;board.thermal={255,255};TssiTracking tracking{};
 const auto trim=rtl8922a_linux_host_default_tssi_trim();
 const auto packet=encode_prepared_tssi_normal_h2c(42,{0,35,41,0},0,{0,0,1},1,0,board,trim,tracking,{true,true,true,true,true});
 assert(packet.size()==308 && packet[8]==0x2c && packet[9]==1);
 const auto scan=encode_prepared_tssi_scan_h2c(42,{0,35,41,0},0,{0,0,1},1,0,board,trim,tracking,{true,true,true,true,true});
 auto expected_scan=packet;expected_scan[306]=1;assert(scan==expected_scan);
 rejects([&]{encode_prepared_tssi_normal_h2c(43,{0,35,41,0},0,{0,0,1},1,0,board,trim,tracking,{true,true,false,true,true});});
 rejects([&]{encode_rfk_h2c(RfkCommand::tssi,44,{});});
 std::cout<<"TX RFK report tests passed\n";
}
