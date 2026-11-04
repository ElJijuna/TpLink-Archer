#include "archer/eapol_m1.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include <iomanip>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: station-capture-audit CAPTURE_DIRECTORY\n";return 2;}
 try{
  std::map<unsigned,fs::path> paths;
  const std::regex pattern("rx-([0-9]+)\\.bin");
  for(const auto& entry:fs::directory_iterator(argv[1])){
   std::smatch m;const auto name=entry.path().filename().string();
   if(std::regex_match(name,m,pattern)){
    auto index=std::stoul(m[1]);if(index>=4096||!entry.is_regular_file()||!paths.emplace(index,entry.path()).second)throw std::runtime_error("Capture identity/bound invalid");
   }
  }
  if(paths.empty())throw std::runtime_error("Empty capture");
  unsigned expected=0,normal=0,messages=0;
  for(const auto& [index,path]:paths){
   if(index!=expected++||fs::file_size(path)>1048576)throw std::runtime_error("Missing capture or excessive extent");
   std::ifstream file(path,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
   if(file.bad()||bytes.empty())throw std::runtime_error("Capture read failed");
   for(const auto& meta:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended))if(meta.normal_80211_candidate()){
    ++normal;const auto* p=bytes.data()+meta.packet.payload_offset;const auto n=meta.packet.payload_bytes;
    if(const auto m1=archer::decode_target_eapol_m1(p,n)){
     ++messages;
     std::cout<<"{\"operation\":\"eapol_m1_capture\",\"file\":\""<<path.filename().string()<<"\",\"payload_offset\":"<<meta.packet.payload_offset
      <<",\"payload_bytes\":"<<n<<",\"key_data_bytes\":"<<m1->key_data_bytes<<",\"frame_with_fcs_hex\":\"";
     for(std::size_t i=0;i<n;++i)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(p[i]);
     std::cout<<std::dec<<"\",\"security_established\":false}"<<std::endl;
    }
   }
  }
  std::cout<<"{\"result\":\"eapol_capture_audited\",\"transfers\":"<<paths.size()<<",\"normal_candidates\":"<<normal<<",\"target_m1_frames\":"<<messages<<",\"security_established\":false}"<<std::endl;
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
