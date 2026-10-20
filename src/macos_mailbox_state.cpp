#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
int main(int argc,char** argv){
    if(argc!=1){std::cout<<"Usage: archer-mailbox-state (run from project root)\n";return argc==2&&std::string(argv[1])=="--help"?0:2;}
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");MacRegisterBus bus;
        archer::with_wlan_io_window(bus,[&]{
            const auto h2c=bus.read(0x7160,1),c2h=bus.read(0x7164,1);
            for(auto a:{0x1f4u,0x1f5u})(void)bus.read(a,1);
            for(auto a:{0x7140u,0x7150u,0x84u,0x842cu})(void)bus.read(a,4);
            std::cout<<"{\"result\":\"mailbox_state\",\"h2c_control\":"<<h2c<<",\"c2h_control\":"<<c2h<<"}\n";
        });
        return 0;
    }catch(const std::exception& e){std::cerr<<"mailbox-state: "<<e.what()<<'\n';return 2;}
}
