#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/xtal_identity.hpp"
#include "archer/led_window.hpp"

int main(int argc,char** argv) {
    const bool long_pulse=argc==2&&std::string(argv[1])=="--gpio8-pulse-15s";
    const bool pulse=long_pulse||(argc==2&&std::string(argv[1])=="--gpio8-pulse-3s");
    const unsigned seconds=long_pulse?15:3;
    const bool inspect=argc==2&&std::string(argv[1])=="--inspect";
    if(!pulse&&!inspect) {
        std::cout<<"Usage: archer-led --inspect | --gpio8-pulse-3s | --gpio8-pulse-15s\n";return 2;
    }
    try {
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        MacRegisterBus bus;
        archer::with_wlan_io_window(bus,[&] {
            auto id=archer::read_chip_identity(bus);
            if(id.revision!=1||id.analog_id!=0x6933)
                throw std::runtime_error("LED experiment requires observed CV1/AID6933 profile");
            for(auto r:{0x2d4u,0x61u,0x62u}) (void)bus.read(r,r==0x62?2:1);
            if(pulse) {
                archer::with_gpio8_led(bus,[&] {
                    std::cout<<"{\"operation\":\"led_gpio8_window\",\"seconds\":"<<seconds<<",\"visual_confirmation_required\":true}"<<std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(seconds));
                });
                std::cout<<"{\"operation\":\"led_registers_restored\"}"<<std::endl;
            }
        });
        return 0;
    } catch(const std::exception& e) {std::cerr<<"led: "<<e.what()<<'\n';return 2;}
}
