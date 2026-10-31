#include "archer/passive_scan.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <set>
// Offline only: software FCS, exact target SSID/BSSID and test destination MAC.
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: archer-special-probe-audit CAPTURE_DIRECTORY\n";return 2;}
 try{
  unsigned bad=0,accepted=0,target_beacons=0,target_responses=0,to_station=0,target_to_station=0;
  const std::array<std::uint8_t,6> station{2,0,0,0x89,0x22,0xa1},target{0x50,0x92,0x6a,0x11,0x06,0x66};
  const std::vector<std::uint8_t> ssid{'E','s','p','e','c','i','a','l'};
  std::set<unsigned> target_rates,target_basic,missing_basic;
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
    if(match){
     for(std::size_t at=36;at<n-4;){
      const auto id=p[at++],len=p[at++];
      if(id==1||id==50)for(unsigned j=0;j<len;++j){
       const unsigned rate=p[at+j]&127;target_rates.insert(rate);
       if(p[at+j]&128){target_basic.insert(rate);
        if(rate!=2&&rate!=4&&rate!=11&&rate!=22)missing_basic.insert(rate);
       }
      }
      at+=len;
     }
     if(b.probe_response)++target_responses;else ++target_beacons;}
    if(b.probe_response&&std::equal(station.begin(),station.end(),p+4)){
     ++to_station;if(match)++target_to_station;
    }
   }
  }
  std::cout<<"{\"receive_channel\":9,\"target_ssid\":\"Especial\",\"target_bssid\":\"50926a110666\",\"accepted_management\":"<<accepted
   <<",\"bad_fcs\":"<<bad<<",\"target_beacons\":"<<target_beacons<<",\"target_probe_responses\":"<<target_responses
   <<",\"probe_responses_to_test_mac\":"<<to_station<<",\"target_probe_responses_to_test_mac\":"<<target_to_station
   <<",\"absence_proves_tx_failure\":false";
  const auto rates=[&](const char* name,const std::set<unsigned>& values){
   std::cout<<",\""<<name<<"\":[";bool first=true;
   for(auto v:values){if(!first)std::cout<<',';first=false;std::cout<<v;}std::cout<<']';
  };
  rates("target_rates_half_mbps",target_rates);rates("target_basic_rates_half_mbps",target_basic);
  rates("target_basic_rates_missing_from_probe",missing_basic);std::cout<<"}\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
