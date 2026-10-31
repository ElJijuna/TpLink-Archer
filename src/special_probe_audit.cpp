#include "archer/passive_scan.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
// Offline only: software FCS, exact target SSID/BSSID and test destination MAC.
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: archer-special-probe-audit CAPTURE_DIRECTORY\n";return 2;}
 try{
  unsigned bad=0,accepted=0,target_beacons=0,target_responses=0,to_station=0,target_to_station=0;
  const std::array<std::uint8_t,6> station{2,0,0,0x89,0x22,0xa1},target{0x50,0x92,0x6a,0x11,0x06,0x66};
  const std::vector<std::uint8_t> ssid{'E','s','p','e','c','i','a','l'};
  unsigned count=0;
  for(const auto& e:std::filesystem::directory_iterator(argv[1]))
   if(e.path().filename().string().rfind("rx-",0)==0&&e.path().extension()==".bin")++count;
  if(!count||count>1024)throw std::runtime_error("Expected1..1024 captures");
  for(unsigned i=0;i<count;++i){
   std::ifstream f(std::filesystem::path(argv[1])/("rx-"+std::to_string(i)+".bin"),std::ios::binary|std::ios::ate);
   if(!f||f.tellg()<0||f.tellg()>20480)throw std::runtime_error("Invalid capture extent");
   f.seekg(0);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(f),{}};
   for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
    if(!m.normal_80211_candidate()||m.packet.payload_bytes<4)continue;
    const auto* p=bytes.data()+m.packet.payload_offset;const auto n=m.packet.payload_bytes;
    if(archer::ieee_fcs(p,n-4)!=archer::channel_u32(p+n-4)){++bad;continue;}
    archer::BeaconSummary b;
    try{b=archer::decode_rx_beacon(bytes,m);}catch(const std::invalid_argument&){continue;}
    ++accepted;
    const bool match=b.bssid==target&&b.ssid==ssid&&b.advertised_ds_channel==9;
    if(match){if(b.probe_response)++target_responses;else ++target_beacons;}
    if(b.probe_response&&std::equal(station.begin(),station.end(),p+4)){
     ++to_station;if(match)++target_to_station;
    }
   }
  }
  std::cout<<"{\"receive_channel\":9,\"target_ssid\":\"Especial\",\"target_bssid\":\"50926a110666\",\"accepted_management\":"<<accepted
   <<",\"bad_fcs\":"<<bad<<",\"target_beacons\":"<<target_beacons<<",\"target_probe_responses\":"<<target_responses
   <<",\"probe_responses_to_test_mac\":"<<to_station<<",\"target_probe_responses_to_test_mac\":"<<target_to_station
   <<",\"absence_proves_tx_failure\":false}\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
