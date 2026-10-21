#include "archer/tssi_protocol.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
template<class F> void rejects(F f) {bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream f(path,std::ios::binary);assert(f.good());return {std::istreambuf_iterator<char>(f),{}};
}
int main() {
    using namespace archer;
    assert(tssi_ofdm_group(42)==(0x80000000u|5));
    assert(tssi_6g_ofdm_group(232)==(0x80000000u|28));
    assert(tssi_trim_group(50)==(0x80000000u|2));
    assert(tssi_6g_trim_group(48)==(0x80000000u|2));
    assert(tssi_group_value(std::array<int,2>{-3,0},0x80000000u)==-1); // C++ truncation toward zero.
    // Portable synthetic board; optional local captures are an additional check.
    std::vector<std::uint8_t> rfmap(0x240);
    rfmap[0xca]=1;rfmap[0xd0]=111;rfmap[0xd1]=113;
    rfmap[0x10]=6;rfmap[0x12]=6;rfmap[0x3c]=18;rfmap[0x3f]=19;
    rfmap[0x18]=12;rfmap[0x42]=21;rfmap[0x22]=241;rfmap[0x23]=247;rfmap[0x100]=251;
    auto board=decode_tssi_board(rfmap);
    assert(board.rfe==1 && board.thermal[0]==111 && board.thermal[1]==113);
    assert(board.cck[0][0]==6 && board.cck[1][5]==19);
    assert(board.mcs[0][5]==-15 && board.mcs6g[0][0]==-5);
    std::vector<std::uint8_t> element(1112);
    channel_put32(element.data(),18);channel_put32(element.data()+4,1080);
    channel_put32(element.data()+24,0xffff);
    if(std::ifstream("artifacts/rtw8922a_fw-1.bin").good() && std::ifstream("artifacts/efuse-stage13/rf-logical.bin").good()) {
        board=decode_tssi_board(read("artifacts/efuse-stage13/rf-logical.bin"));
        const auto fw=read("artifacts/rtw8922a_fw-1.bin");
        const unsigned offset=1580656;
        assert(fw.size()>=offset+1112);
        element.assign(fw.begin()+offset,fw.begin()+offset+1112);
    }
    auto track=decode_tssi_tracking_element(element,0x6933);
    const TssiTrimData zero_host_trim(std::array<std::array<int,8>,2>{},
                                      std::array<std::array<int,16>,2>{});
    const RfkFirmwareVersion version{0,35,41,0};
    auto payload=prepare_tssi_normal_payload(version,0,{0,0,6},1,0,board,zero_host_trim,track);
    assert(payload.size()==300 && payload[0]==44 && payload[1]==1 && payload[6]==1);
    assert(payload[8]==0 && payload[9]==0 && payload[10]==6 && payload[11]==18);
    assert(payload[14]==6 && payload[15]==18 && payload[16]==0 && payload[17]==0);
    for(unsigned i=5;i<=9;++i)assert(payload[8+2*i]==12 && payload[8+2*i+1]==21);
    assert(payload[40]==111 && payload[41]==113 && payload[298]==0 && payload[299]==1);
    // Synthetic signed table validates clipping, negative-side indexing and reversed chunks.
    for(unsigned type=0;type<16;++type)for(auto& band:track.delta[type])
        for(unsigned i=0;i<30;++i)band[i]=int(i);
    payload=prepare_tssi_normal_payload(version,0,{0,0,6},1,0,board,zero_host_trim,track);
    assert(payload[42]==3 && payload[45]==0);
    assert(payload[42+60]==29 && payload[42+64]==std::uint8_t(-29));
    assert(payload[42+124]==std::uint8_t(-1) && payload[42+127]==std::uint8_t(-4));
    auto interpolated=prepare_tssi_normal_payload(version,0,{1,2,42},1,0,board,zero_host_trim,track);
    assert(interpolated[28]==std::uint8_t((-15-9)/2)); // efuse OFDM DE field10 pathA
    auto erased_thermal=board;erased_thermal.thermal[1]=255;
    auto fallback=prepare_tssi_normal_payload(version,0,{0,0,6},1,0,erased_thermal,zero_host_trim,track);
    assert(fallback[41]==0x38);
    for(unsigned i=170;i<298;++i)assert(fallback[i]==0);
    rejects([&]{decode_tssi_board(std::vector<std::uint8_t>(0x240,255));});
    rejects([&]{decode_tssi_board(std::vector<std::uint8_t>(0x239,0));});
    auto foreign=element;foreign[12]=1;foreign[13]=1;
    rejects([&]{decode_tssi_tracking_element(foreign,0x6933);});
    auto bad=element;bad[24]=0xfe;
    rejects([&]{decode_tssi_tracking_element(bad,0x6933);});
    rejects([&]{decode_tssi_tracking_element({},0x6933);});
    rejects([&]{prepare_tssi_normal_payload(version,0,{0,0,6},1,1,board,zero_host_trim,track);});
    auto invalid=track;invalid.delta[0][0][0]=128;
    rejects([&]{prepare_tssi_normal_payload(version,0,{0,0,6},1,0,board,zero_host_trim,invalid);});
    // Preparation never removes the existing hardware-send prohibition.
    rejects([&]{encode_rfk_h2c(RfkCommand::tssi,0,payload);});
    std::cout<<"TSSI offline preparation tests passed\n";
}
