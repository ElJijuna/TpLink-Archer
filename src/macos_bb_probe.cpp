#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/bb_window.hpp"
#include "archer/rf_transport.hpp"
#include "archer/hwsi_window.hpp"
#include "archer/rf_power.hpp"
#include "archer/bb_table_window.hpp"
#include "bb_payload.hpp"
int main(int argc,char** argv){
    const bool full_table=argc==2&&std::string(argv[1])=="--rf-power-bb-table";
    const bool power=argc==2&&std::string(argv[1])=="--rf-power-window";
    const bool table=argc==2&&std::string(argv[1])=="--table-hwsi-control";
    const bool clock=argc==2&&std::string(argv[1])=="--clock-hwsi-control";
    const bool prerequisites=argc==2&&std::string(argv[1])=="--inspect-prerequisites";
    if(argc!=1&&!table&&!prerequisites&&!clock&&!power&&!full_table){std::cout<<"Usage: archer-bb-probe [--table-hwsi-control | --inspect-prerequisites | --clock-hwsi-control | --rf-power-window | --rf-power-bb-table] (run from project root)\n";return argc==2&&std::string(argv[1])=="--help"?0:2;}
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        (void)archer::query_phy_capabilities_with_io_window(bus,[&]{
            const auto identity=archer::read_chip_identity(bus);
            if(identity.analog_id!=0x6933)throw std::runtime_error("Unexpected analog chip identifier");
            auto bb_probe=[&]{ archer::with_bb_diagnostic_window(bus,[&]{
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
                    for(auto address:full_table?std::vector<unsigned>{0,5,0x18,0x42}:std::vector<unsigned>{0}){
                        const auto value=rf.read_full(path,address);
                        std::cout<<"{\"result\":\"rf_read\",\"path\":"<<path<<",\"address\":"<<address<<",\"value\":"<<value<<"}\n";
                    }
                }
                };
                if(full_table)archer::with_bb_table_window(bus,trusted_bb_table,read_rf);
                else if(clock||power)archer::with_bb_clock_window(bus,[&]{archer::with_hwsi_table_window(bus,read_rf);});
                else if(table)archer::with_hwsi_table_window(bus,read_rf);else read_rf();
            }); };
            if(power||full_table)archer::with_rf_power_window(bus,[&]{
                (void)bus.read(0x888,4);bb_probe();
            });else bb_probe();
        });
        return 0;
    }catch(const std::exception& e){std::cerr<<"bb-probe: "<<e.what()<<'\n';return 2;}
}
