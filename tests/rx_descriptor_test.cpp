#include "archer/rx_descriptor.hpp"
#include <cassert>
#include <iostream>
void word(std::vector<std::uint8_t>& b,unsigned at,unsigned v){for(unsigned i=0;i<4;++i)b[at+i]=(v>>(i*8))&255;}
template<class F>void rejects(F f){bool x=false;try{f();}catch(const std::exception&){x=true;}assert(x);}
int main(){
 std::vector<std::uint8_t>b(48);word(b,0,24);word(b,12,1u<<6);word(b,16,(0xabcU<<20)|(3u<<12));
 auto m=archer::decode_be_rx_metadata(b);assert(m.size()==1&&m[0].descriptor_bytes==24);
 assert(m[0].crc_error&&!m[0].icv_error&&m[0].rate_code==0xabc&&m[0].bandwidth_code==3);
 assert(!m[0].header_converted.has_value()&&!m[0].normal_80211_candidate(true));
 rejects([&]{m[0].payload_without_fcs_bytes();});
 word(b,12,0);m=archer::decode_be_rx_metadata(b,archer::RxFcsPolicy::appended);
 assert(!m[0].normal_80211_candidate()&&m[0].normal_80211_candidate(true));
 assert(m[0].payload_without_fcs_bytes()==20);
 b.resize(64);word(b,0,0x80000000|24);word(b,28,1u<<16);word(b,36,(17u<<16)|(26u<<21));
 m=archer::decode_be_rx_metadata(b);assert(m[0].descriptor_bytes==40&&*m[0].header_converted);
 assert(*m[0].header_offset==17&&*m[0].wireless_header_iv_bytes==26&&!m[0].normal_80211_candidate(true));
 word(b,28,0);m=archer::decode_be_rx_metadata(b);assert(m[0].normal_80211_candidate());
 word(b,12,1u<<7);m=archer::decode_be_rx_metadata(b);assert(m[0].icv_error&&!m[0].normal_80211_candidate());
 b.resize(39);rejects([&]{archer::decode_be_rx_metadata(b);});
 std::cout<<"RX descriptor tests passed\n";
}
