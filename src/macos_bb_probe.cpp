#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/bb_window.hpp"
#include "archer/rf_transport.hpp"
#include "archer/hwsi_window.hpp"
int main(int argc,char** argv){
    const bool table=argc==2&&std::string(argv[1])=="--table-hwsi-control";
    const bool clock=argc==2&&std::string(argv[1])=="--clock-hwsi-control";
    const bool prerequisites=argc==2&&std::string(argv[1])=="--inspect-prerequisites";
    if(argc!=1&&!table&&!prerequisites&&!clock){std::cout<<"Usage: archer-bb-probe [--table-hwsi-control | --inspect-prerequisites | --clock-hwsi-control] (run from project root)\n";return argc==2&&std::string(argv[1])=="--help"?0:2;}
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        (void)archer::query_phy_capabilities_with_io_window(bus,[&]{
            const auto identity=archer::read_chip_identity(bus);
            if(identity.analog_id!=0x6933)throw std::runtime_error("Unexpected analog chip identifier");
            archer::with_bb_diagnostic_window(bus,[&]{
                if(prerequisites){
                    for(auto address:{0xd0u,0x84u,0x20000u,0x20704u,0x22adcu,0x22bdcu,0x22c24u,0x22d24u})
                        (void)bus.read(address,4);
                    return;
                }
                for(auto address:{0x22c24u,0x22d24u,0x22adcu,0x22bdcu}){
                    const auto value=bus.read(address,4);
                    if(value==0xeaeaeaea || value==0xdeadbeef)
                        throw std::runtime_error("HWSI still inaccessible after BB reset release");
                }
                auto read_rf=[&]{
                archer::BeRfTransport rf(bus);
                for(unsigned path=0;path<2;++path){
                    const auto value=rf.read_full(path,0);
                    std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":0,\"value\":"<<value<<"}\n";
                }
                };
                if(clock)archer::with_bb_clock_window(bus,[&]{archer::with_hwsi_table_window(bus,read_rf);});
                else if(table)archer::with_hwsi_table_window(bus,read_rf);else read_rf();
            });
        });
        return 0;
    }catch(const std::exception& e){std::cerr<<"bb-probe: "<<e.what()<<'\n';return 2;}
}
