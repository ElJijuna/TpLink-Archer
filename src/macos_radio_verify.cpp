#include "archer/macos_register_bus.hpp"
#include "archer/rf_transport.hpp"
#include "archer/sequence_ledger.hpp"
int main(){
 try {
    archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
    MacRegisterBus bus;
    archer::with_wlan_io_window(bus,[&]{
        archer::BeRfTransport rf(bus);
        bool expected=true;
        for(unsigned p=0;p<2;++p)for(auto a:{0u,5u,0x18u,0x10018u,0x42u}){
            const auto value=rf.read_full(p,a);
            std::cout<<"{\"result\":\"rf_read\",\"path\":"<<p<<",\"address\":"<<a<<",\"value\":"<<value<<"}"<<std::endl;
            // RF0 mode changes asynchronously after NCTL/firmware startup.
            if(a==5 && value!=1)expected=false;
        }
        for(auto a:{0x8c08u,0x9008u,0x8d00u,0x9100u,0x9400u}) {
            const auto value=bus.read(a,4);
            std::cout<<"{\"result\":\"mac_read\",\"address\":"<<a<<",\"value\":"<<value<<"}"<<std::endl;
        }
        if(!expected)throw std::runtime_error("Radio table final values do not match");
        std::cout<<"{\"result\":\"radio_final_values_verified\"}"<<std::endl;
    });
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
