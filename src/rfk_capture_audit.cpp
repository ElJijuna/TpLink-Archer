#include "archer/tx_rfk.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
template<class T>void array(const char* name,const T& a){
 std::cout<<",\""<<name<<"\":[";bool first=true;
 for(auto v:a){if(!first)std::cout<<',';first=false;std::cout<<v;}std::cout<<']';
}
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: rfk-capture-audit CAPTURE_DIRECTORY\n";return 2;}
 try{
  unsigned count=0,dpk=0,tssi=0;
  for(const auto& e:std::filesystem::directory_iterator(argv[1]))if(e.path().filename().string().rfind("rx-",0)==0&&e.path().extension()==".bin")++count;
  if(!count||count>1024)throw std::runtime_error("Expected1..1024 captures");
  for(unsigned i=0;i<count;++i){
   std::ifstream f(std::filesystem::path(argv[1])/("rx-"+std::to_string(i)+".bin"),std::ios::binary|std::ios::ate);
   if(!f||f.tellg()<0||f.tellg()>20480)throw std::runtime_error("Invalid capture extent");
   f.seekg(0);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(f),{}};
   for(const auto& p:archer::decode_usb_rx_be(bytes))if(p.c2h){
    const auto* at=bytes.data()+p.payload_offset;
    for(const auto& r:archer::decode_dpk_log_reports(at,p.payload_bytes)){
     ++dpk;std::cout<<"{\"report\":\"dpk\",\"capture\":"<<i<<",\"version\":"<<r.version<<",\"path_success\":"<<(r.path_success().has_value()?(*r.path_success()?"true":"false"):"null")<<",\"quality_validated\":false";
     array("channel",r.channel);array("path_status",r.path_ok);array("index",r.index);array("txagc_raw",r.txagc);array("thermal_raw",r.thermal);
     array("gain_scaling_raw",r.gain_scaling);array("dc_i_bytes",r.dc_i_bytes);array("dc_q_bytes",r.dc_q_bytes);
     array("correlation_value_raw",r.correlation_value);array("correlation_index_raw",r.correlation_index);
     array("timeout",r.timeout);array("rxbb_overflow",r.rxbb_overflow);std::cout<<"}\n";
    }
    for(const auto& r:archer::decode_tssi_log_reports(at,p.payload_bytes)){
     ++tssi;std::cout<<"{\"report\":\"tssi\",\"capture\":"<<i<<",\"quality_validated\":false,\"alignment_codes_all_zero\":"
      <<(std::all_of(r.alignment_code.begin(),r.alignment_code.end(),[](auto x){return x==0;})?"true":"false");
     array("alignment_power_raw",r.alignment_power);array("alignment_code",r.alignment_code);array("alignment_state",r.alignment_state);
     array("default_txagc_offset_raw",r.default_txagc_offset);std::cout<<"}\n";
    }
   }
  }
  std::cout<<"{\"result\":\"rfk_capture_audit_completed\",\"dpk_reports\":"<<dpk<<",\"tssi_reports\":"<<tssi<<",\"hardware_access\":false}"<<std::endl;
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
