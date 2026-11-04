#include "archer/macos_bulk.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/rx_descriptor.hpp"
#include <filesystem>
#include <fstream>
// Passive late-delivery audit: no send(), reserve(), filter changes or CLEAN.
int main(int argc,char** argv){
 if(argc!=3||std::string(argv[1])!="--read-only-2s"){std::cerr<<"Usage: pending-rx --read-only-2s NEW_DIRECTORY\n";return 2;}
 try{
  archer::SequenceLedger owner("artifacts/37ad-0104-sequences.bin");
  const std::filesystem::path dir=argv[2];if(!std::filesystem::create_directory(dir))throw std::runtime_error("New directory required");
  MacBulkChannel usb(true);unsigned transfers=0,c2h=0;std::map<unsigned,unsigned> types;
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(std::chrono::steady_clock::now()<end)for(unsigned pipe=0;pipe<2;++pipe){
   auto bytes=usb.receive(pipe,usb.has_completed_input(1-pipe)?0:5);if(bytes.empty())continue;
   if(transfers>=4096)throw std::runtime_error("Passive capture bound reached");
   std::ofstream out(dir/("rx-"+std::to_string(transfers++)+".bin"),std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());out.close();if(!out)throw std::runtime_error("Passive capture write failed");
   for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended)){
    ++types[m.packet.packet_type];if(m.packet.c2h){++c2h;std::cout<<"{\"operation\":\"passive_c2h\",\"category\":"<<m.packet.c2h->category<<",\"class\":"<<m.packet.c2h->command_class<<",\"function\":"<<m.packet.c2h->function<<",\"transfer\":"<<(transfers-1)<<"}"<<std::endl;}
   }
  }
  usb.close();std::cout<<"{\"result\":\"passive_late_rx_complete\",\"transfers\":"<<transfers<<",\"c2h_packets\":"<<c2h<<",\"packet_types\":{";bool comma=false;for(const auto& [type,count]:types){if(comma)std::cout<<',';comma=true;std::cout<<'\"'<<type<<"\":"<<count;}std::cout<<"},\"h2c_submissions\":0,\"network_submissions\":0,\"firmware_resets\":0}"<<std::endl;return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
