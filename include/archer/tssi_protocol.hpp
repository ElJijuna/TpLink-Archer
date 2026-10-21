#pragma once
#include "rfk_protocol.hpp"
#include <array>
namespace archer {
// Offline payload preparation only. Existing RFK send guard remains closed.
struct TssiGroupRange { unsigned first, last, index; bool average; };
template<std::size_t N> inline unsigned tssi_group(unsigned channel,
        const TssiGroupRange (&ranges)[N]) {
    for(const auto& r:ranges)if(channel>=r.first && channel<=r.last)
        return r.index | (r.average ? 0x80000000u : 0);
    return 0; // Same source fallback; not regulatory validation.
}
inline unsigned tssi_cck_group(unsigned channel) {
    constexpr TssiGroupRange ranges[]{
{1,2,0,false},{3,5,1,false},{6,8,2,false},{9,11,3,false},{12,13,4,false},{14,14,5,false}};
    return tssi_group(channel,ranges);
}
inline unsigned tssi_ofdm_group(unsigned channel) {
    constexpr TssiGroupRange ranges[]{
{1,2,0,false},{3,5,1,false},{6,8,2,false},{9,11,3,false},{12,14,4,false},{36,40,5,false},{41,43,5,true},{44,48,6,false},{49,51,6,true},{52,56,7,false},{57,59,7,true},{60,64,8,false},{100,104,9,false},{105,107,9,true},{108,112,10,false},{113,115,10,true},{116,120,11,false},{121,123,11,true},{124,128,12,false},{129,131,12,true},{132,136,13,false},{137,139,13,true},{140,144,14,false},{149,153,15,false},{154,156,15,true},{157,161,16,false},{162,164,16,true},{165,169,17,false},{170,172,17,true},{173,177,18,false}};
    return tssi_group(channel,ranges);
}
inline unsigned tssi_6g_ofdm_group(unsigned channel) {
    constexpr TssiGroupRange ranges[]{
{1,5,0,false},{6,8,0,true},{9,13,1,false},{14,16,1,true},{17,21,2,false},{22,24,2,true},{25,29,3,false},{33,37,4,false},{38,40,4,true},{41,45,5,false},{46,48,5,true},{49,53,6,false},{54,56,6,true},{57,61,7,false},{65,69,8,false},{70,72,8,true},{73,77,9,false},{78,80,9,true},{81,85,10,false},{86,88,10,true},{89,93,11,false},{97,101,12,false},{102,104,12,true},{105,109,13,false},{110,112,13,true},{113,117,14,false},{118,120,14,true},{121,125,15,false},{129,133,16,false},{134,136,16,true},{137,141,17,false},{142,144,17,true},{145,149,18,false},{150,152,18,true},{153,157,19,false},{161,165,20,false},{166,168,20,true},{169,173,21,false},{174,176,21,true},{177,181,22,false},{182,184,22,true},{185,189,23,false},{193,197,24,false},{198,200,24,true},{201,205,25,false},{206,208,25,true},{209,213,26,false},{214,216,26,true},{217,221,27,false},{225,229,28,false},{230,232,28,true},{233,237,29,false},{238,240,29,true},{241,245,30,false},{246,248,30,true},{249,253,31,false}};
    return tssi_group(channel,ranges);
}
inline unsigned tssi_trim_group(unsigned channel) {
    constexpr TssiGroupRange ranges[]{
{1,8,0,false},{9,14,1,false},{36,48,2,false},{49,51,2,true},{52,64,3,false},{100,112,4,false},{113,115,4,true},{116,128,5,false},{132,144,6,false},{149,177,7,false}};
    return tssi_group(channel,ranges);
}
inline unsigned tssi_6g_trim_group(unsigned channel) {
    constexpr TssiGroupRange ranges[]{
{1,13,0,false},{14,16,0,true},{17,29,1,false},{33,45,2,false},{46,48,2,true},{49,61,3,false},{65,77,4,false},{78,80,4,true},{81,93,5,false},{97,109,6,false},{110,112,6,true},{113,125,7,false},{129,141,8,false},{142,144,8,true},{145,157,9,false},{161,173,10,false},{174,176,10,true},{177,189,11,false},{193,205,12,false},{206,208,12,true},{209,221,13,false},{225,237,14,false},{238,240,14,true},{241,253,15,false}};
    return tssi_group(channel,ranges);
}
inline int tssi_signed(std::uint8_t byte) { return byte < 128 ? byte : int(byte)-256; }
template<std::size_t N> inline int tssi_group_value(const std::array<int,N>& values,unsigned group) {
    const unsigned index=group&0x7fffffffu;
    if(index>=N || ((group&0x80000000u) && index+1>=N))
        throw std::invalid_argument("TSSI group exceeds source data");
    return (group&0x80000000u) ? (values[index]+values[index+1])/2 : values[index];
}
struct TssiBoardData {
    unsigned rfe;
    std::array<unsigned,2> thermal;
    std::array<std::array<int,6>,2> cck;
    std::array<std::array<int,19>,2> mcs;
    std::array<std::array<int,32>,2> mcs6g;
};
inline TssiBoardData decode_tssi_board(const std::vector<std::uint8_t>& rf_logical) {
    if(rf_logical.size()!=0x240 || std::all_of(rf_logical.begin(),rf_logical.end(),
        [](unsigned b){return b==255;}))throw std::invalid_argument("Missing RTL8922A logical RF map");
    TssiBoardData board{}; board.rfe=rf_logical[0xca];
    if(board.rfe==255)throw std::invalid_argument("Erased RTL8922A RFE identity");
    for(unsigned p=0;p<2;++p) {
        const unsigned base=p?0x3a:0x10;
        board.thermal[p]=rf_logical[0xd0+p];
        for(unsigned i=0;i<6;++i)board.cck[p][i]=tssi_signed(rf_logical[base+i]);
        for(unsigned i=0;i<5;++i)board.mcs[p][i]=tssi_signed(rf_logical[base+6+i]);
        for(unsigned i=0;i<14;++i)board.mcs[p][5+i]=tssi_signed(rf_logical[base+18+i]);
        for(unsigned i=0;i<32;++i)board.mcs6g[p][i]=tssi_signed(rf_logical[0x100+p*42+i]);
    }
    return board;
}
struct TssiTracking {
    std::array<std::array<std::array<int,30>,4>,16> delta{};
};
inline TssiTracking decode_tssi_tracking_element(const std::vector<std::uint8_t>& element,
                                                unsigned chip_aid) {
    if(chip_aid>65535 || element.size()!=1112 || channel_u32(element.data())!=18 ||
       channel_u32(element.data()+4)!=1080 || channel_u32(element.data()+24)!=0xffff)
        throw std::invalid_argument("Expected complete RTL8922A TXPWR_TRK element bitmap ffff");
    const unsigned aid=element[12]|(unsigned(element[13])<<8);
    if(aid && aid!=chip_aid)throw std::invalid_argument("Foreign TXPWR_TRK AID");
    TssiTracking tracking;
    unsigned cursor=32;
    for(unsigned type=0;type<16;++type)for(unsigned band=0;band<(type<4?4u:type<8?3u:1u);++band)
        for(unsigned i=0;i<30;++i)tracking.delta[type][band][i]=tssi_signed(element[cursor++]);
    return tracking;
}
struct TssiTrimData {
    std::array<std::array<int,8>,2> trim;
    std::array<std::array<int,16>,2> trim6g;
    TssiTrimData()=delete;
    TssiTrimData(std::array<std::array<int,8>,2> a,std::array<std::array<int,16>,2> b)
        :trim(a),trim6g(b){}
};
inline void validate_tssi_signed(int value) {
    if(value < -128 || value > 127)throw std::invalid_argument("TSSI signed factory/table code out of range");
}
// thermal_group is selected by the caller from validated channel subband context:
// 2G:0; 5G bands1/3/4:0/1/2; 6G subbands0..7:0/0/1/1/2/2/3/3.
inline std::vector<std::uint8_t> prepare_tssi_normal_payload(RfkFirmwareVersion version,
        unsigned phy,const RfkChannel& channel,unsigned cv,unsigned thermal_group,
        const TssiBoardData& board,const TssiTrimData& trim,const TssiTracking& tracking) {
    require_rfk_v1_firmware(version);validate_rfk_phy(phy);validate_rfk_channel(channel);validate_rfk_cv(cv);
    if(board.rfe>=255 || thermal_group>=(channel.band==0?1u:channel.band==1?3u:4u))
        throw std::invalid_argument("Invalid TSSI RFE or thermal subband group");
    for(unsigned p=0;p<2;++p) {
        if(board.thermal[p]>255)throw std::invalid_argument("TSSI thermal exceeds wire width");
        for(auto x:board.cck[p])validate_tssi_signed(x);
        for(auto x:board.mcs[p])validate_tssi_signed(x);
        for(auto x:board.mcs6g[p])validate_tssi_signed(x);
        for(auto x:trim.trim[p])validate_tssi_signed(x);
        for(auto x:trim.trim6g[p])validate_tssi_signed(x);
    }
    for(const auto& type:tracking.delta)for(const auto& band:type)for(auto x:band)validate_tssi_signed(x);
    std::vector<std::uint8_t> payload(300,0);
    payload[0]=0x2c;payload[1]=1;payload[2]=phy;payload[3]=channel.channel;
    payload[4]=channel.bandwidth;payload[5]=channel.band;payload[6]=1;payload[7]=cv;
    for(unsigned p=0;p<2;++p) {
        const int td=channel.band==2 ? tssi_group_value(trim.trim6g[p],tssi_6g_trim_group(channel.channel)) :
                                                      tssi_group_value(trim.trim[p],tssi_trim_group(channel.channel));
        const int cck=tssi_group_value(board.cck[p],tssi_cck_group(channel.channel));
        const int ofdm=channel.band==2 ? tssi_group_value(board.mcs6g[p],tssi_6g_ofdm_group(channel.channel)) :
                                                       tssi_group_value(board.mcs[p],tssi_ofdm_group(channel.channel));
        // 16 signed two-path DE arrays. Generic CCK/OFDM and four diff arrays remain zero per 8922A source.
        auto de=[&](unsigned field,int value){payload[8+2*field+p]=static_cast<std::uint8_t>(value);};
        de(1,cck+td);de(2,cck+td);de(3,cck);
        for(unsigned field=5;field<=9;++field)de(field,ofdm+td);
        de(10,ofdm);de(15,td);
        const unsigned thermal=board.thermal[p];
        payload[40+p]=thermal==255?0x38:thermal;
        if(thermal==255)continue; // Explicit fallback in source, not inferred factory data.
        const unsigned type_base=channel.band==0?8:channel.band==1?4:0;
        const auto& up=tracking.delta[type_base+(p==0?3:1)][thermal_group];
        const auto& down=tracking.delta[type_base+(p==0?2:0)][thermal_group];
        std::array<int,128> offsets{};
        for(unsigned j=0;j<64;++j)offsets[j]=up[std::min(j,29u)];
        for(unsigned j=64;j<128;++j)offsets[j]=-down[std::min(128-j,29u)];
        for(unsigned i=0;i<128;++i)payload[42+p*128+i]=static_cast<std::uint8_t>(offsets[i^3]);
    }
    payload[298]=0;payload[299]=board.rfe;
    return payload; // Caller cannot send through encode_rfk_h2c until its separate guard is lifted.
}
} // namespace archer
