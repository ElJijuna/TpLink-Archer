#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/xtal_identity.hpp"
int main(int argc,char** argv) {
    if(argc>1) {std::cout<<"Usage: archer-identity (run from project root)\n";return argc==2 && std::string(argv[1])=="--help"?0:2;}
    try {
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        MacRegisterBus bus;
        archer::ChipIdentity identity{};
        (void)archer::query_phy_capabilities_with_io_window(bus,[&]{identity=archer::read_chip_identity(bus);});
        std::cout<<"{\"result\":\"chip_identity\",\"revision\":"<<identity.revision
                 <<",\"analog_revision\":"<<identity.analog_revision<<",\"analog_id\":"<<identity.analog_id<<"}\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<"identity: "<<error.what()<<'\n';return 2;}
}
