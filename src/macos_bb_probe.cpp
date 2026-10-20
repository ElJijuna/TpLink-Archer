#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/bb_window.hpp"
#include "archer/rf_transport.hpp"
int main(int argc,char** argv){
    if(argc!=1){std::cout<<"Usage: archer-bb-probe (run from project root)\n";return argc==2&&std::string(argv[1])=="--help"?0:2;}
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        (void)archer::query_phy_capabilities_with_io_window(bus,[&]{
            const auto identity=archer::read_chip_identity(bus);
            if(identity.analog_id!=0x6933)throw std::runtime_error("Unexpected analog chip identifier");
            archer::with_bb_diagnostic_window(bus,[&]{
                for(auto address:{0x22c24u,0x22d24u,0x22adcu,0x22bdcu}){
                    const auto value=bus.read(address,4);
                    if(value==0xeaeaeaea || value==0xdeadbeef)
                        throw std::runtime_error("HWSI still inaccessible after BB reset release");
                }
                archer::BeRfTransport rf(bus);
                for(unsigned path=0;path<2;++path){
                    const auto value=rf.read_full(path,0);
                    std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":0,\"value\":"<<value<<"}\n";
                }
            });
        });
        return 0;
    }catch(const std::exception& e){std::cerr<<"bb-probe: "<<e.what()<<'\n';return 2;}
}
