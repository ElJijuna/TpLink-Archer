#include "archer/control_frame_audit.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: control-capture-audit CAPTURE_DIRECTORY\n";return 2;}
 try{
  unsigned count=0;archer::ControlFrameAudit audit;
  for(const auto& e:std::filesystem::directory_iterator(argv[1]))if(e.path().filename().string().rfind("rx-",0)==0&&e.path().extension()==".bin")++count;
  if(!count||count>1024)throw std::runtime_error("Expected1..1024 captures");
  for(unsigned i=0;i<count;++i){
   std::ifstream f(std::filesystem::path(argv[1])/("rx-"+std::to_string(i)+".bin"),std::ios::binary|std::ios::ate);
   if(!f||f.tellg()<0||f.tellg()>20480)throw std::runtime_error("Invalid capture extent");
   f.seekg(0);std::vector<std::uint8_t>b{std::istreambuf_iterator<char>(f),{}};
   for(const auto& m:archer::decode_be_rx_metadata(b,archer::RxFcsPolicy::appended))audit.observe(b,m);
  }
  audit.write_json(std::cout);return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
