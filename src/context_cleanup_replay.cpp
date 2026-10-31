#include "archer/interface_lifecycle.hpp"
#include "archer/single_probe.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
static std::vector<std::uint8_t> read(const std::filesystem::path& path){
 std::ifstream f(path,std::ios::binary|std::ios::ate);
 if(!f||f.tellg()<0||f.tellg()>20480)throw std::runtime_error("Missing/oversize capture");
 f.seekg(0);return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv){
 if(argc!=3||std::string(argv[1])!="--stage86"){std::cerr<<"Usage: context-cleanup-replay --stage86 CAPTURE_DIRECTORY\n";return 2;}
 try{
  const std::filesystem::path dir=argv[2];
  const std::array<unsigned,9> seq{41,42,43,44,45,46,47,48,49};
  archer::InterfaceContextLifecycle session({{2,0,0,0x89,0x22,0xa1},0,0,0,0},seq,true,true,false,archer::StationCapability::legacy,0,true);
  if(read(dir/"single-probe.bin")!=archer::special_channel9_unicast_probe_packet())throw std::runtime_error("Not the stage86 unicast packet");
  unsigned issued=0,terminal=0;
  const auto issue=[&]{
   const auto step=session.step();
   if(archer::wrap_h2c_usb_be(session.issue())!=read(dir/("tx-"+std::to_string(step)+".bin")))throw std::runtime_error("Context command differs from source-bound lifecycle");
   session.sent_successfully();++issued;
  };
  issue();unsigned count=0;
  for(const auto& e:std::filesystem::directory_iterator(dir))
   if(e.path().filename().string().rfind("rx-",0)==0&&e.path().extension()==".bin")++count;
  if(!count||count>1024)throw std::runtime_error("Capture count invalid");
  for(unsigned i=0;i<count;++i){
   const auto bytes=read(dir/("rx-"+std::to_string(i)+".bin"));
   for(const auto& p:archer::decode_usb_rx_be(bytes))if(p.c2h){
    const auto* at=bytes.data()+p.payload_offset;
    if(const auto report=archer::single_probe_report(at,p.payload_bytes,3)){
     if(session.phase()!=archer::ContextPhase::ready||report->state!=1||report->count!=1||terminal++)throw std::runtime_error("Unexpected/duplicate unicast terminal");
     session.begin_removal();issue();
    }
    const auto before=session.step();session.observe(at,p.payload_bytes);
    if(session.phase()==archer::ContextPhase::uncertain)throw std::runtime_error("Context report uncertain");
    if(session.step()!=before&&session.step()<9){
     issue(); // Step5 has source submission-only policy; it advances to ready.
    }
   }
  }
  if(session.phase()!=archer::ContextPhase::cleanup_acknowledged||session.step()!=9||issued!=9||terminal!=1)throw std::runtime_error("Incomplete raw cleanup proof");
  std::cout<<"{\"result\":\"stage86_cleanup_replayed\",\"issued_commands\":9,\"final_step\":9,\"retry_limit_terminal\":true,\"pending_h2c\":false}\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
