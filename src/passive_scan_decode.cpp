#include "archer/passive_scan.hpp"
#ifdef ARCHER_PROBE_AIR_AUDIT
#include "archer/probe_air_audit.hpp"
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv){
    const std::string option=argc>1?argv[1]:"";
    const unsigned channel=option=="--channel1-fcs-appended"?1:option=="--channel6-fcs-appended"?6:option=="--channel9-fcs-appended"?9:option=="--channel11-fcs-appended"?11:0;
    if(argc!=3||!channel){
        std::cerr<<"Usage: archer-passive-scan-decode {--channel1-fcs-appended|--channel6-fcs-appended|--channel9-fcs-appended|--channel11-fcs-appended} RX_DIRECTORY\n";return 2;
    }
    try{
#ifdef ARCHER_PROBE_AIR_AUDIT
        archer::ProbeAirAudit scan(channel);
#else
        archer::PassiveScan scan(channel);
#endif
        unsigned total=0;
        for(const auto& e:std::filesystem::directory_iterator(argv[2])){
            const auto name=e.path().filename().string();
            if(name.rfind("rx-",0)==0&&e.path().extension()==".bin")++total;
        }
        if(!total||total>1024)throw std::runtime_error("Expected 1..1024 RX transfers");
        // Exact filenames preserve acquisition order, not lexical rx-10/rx-2 order.
        for(unsigned i=0;i<total;++i){
            const auto path=std::filesystem::path(argv[2])/("rx-"+std::to_string(i)+".bin");
            if(!std::filesystem::exists(path))throw std::runtime_error("Noncontiguous capture filenames");
            std::ifstream file(path,std::ios::binary|std::ios::ate);
            if(!file||file.tellg()<0||file.tellg()>20480)throw std::runtime_error("RX transfer exceeds bound");
            file.seekg(0);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
            if(file.bad())throw std::runtime_error("RX transfer read failed");
            for(const auto& m:archer::decode_be_rx_metadata(bytes,archer::RxFcsPolicy::appended))scan.observe(bytes,m);
        }
        scan.write_json(std::cout);return 0;
    }catch(const std::exception& e){std::cerr<<"passive-scan-decode: "<<e.what()<<'\n';return 2;}
}
