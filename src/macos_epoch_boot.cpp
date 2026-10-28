#include "archer/macos_bulk.hpp"
#include "archer/workflow_lock.hpp"
#include "archer/mac_reset.hpp"
#include "archer/bb_postinit.hpp"
#include "archer/rf_power.hpp"
#include "boot_payload.hpp"
#include <filesystem>
#include <fstream>
// Deliberately no SequenceLedger and no H2C commands. The parent owns and
// freezes the old ledger while this process establishes the firmware boundary.
int main(int argc,char** argv){
    if(argc!=3||std::string(argv[1])!="--reload-powered-no-h2c"){
        std::cerr<<"Usage: archer-epoch-boot --reload-powered-no-h2c NEW_DIRECTORY (coordinator only)\n";return 2;
    }
    try{
        if(!std::getenv("ARCHER_WORKFLOW_FD"))throw std::runtime_error("Exclusive coordinator required");
        archer::WorkflowLock workflow("artifacts/37ad-0104-sequences.bin.workflow");
        const char* value=std::getenv("ARCHER_EPOCH_LEDGER_FD");char* end=nullptr;
        if(!value||!*value)throw std::runtime_error("Inherited old ledger required");
        errno=0;const long fd=std::strtol(value,&end,10);struct stat held{},target{};
        if(errno||*end||fd<0||fd>0x7fffffff||fstat(fd,&held)||lstat("artifacts/37ad-0104-sequences.bin",&target)||
           !S_ISREG(target.st_mode)||target.st_nlink!=1||held.st_ino!=target.st_ino||held.st_dev!=target.st_dev||flock(fd,LOCK_EX|LOCK_NB))
            throw std::runtime_error("Old ledger ownership mismatch");
        const int fence=open("artifacts/37ad-0104-sequences.bin.epoch-transition",O_RDONLY|O_NOFOLLOW);
        char fence_state[7]{};const auto count=fence<0?-1:read(fence,fence_state,7);if(fence>=0)close(fence);
        if(count!=6||std::string(fence_state,6)!="DIRTY\n")throw std::runtime_error("Durable transition fence required");
        const auto images=trusted_boot_images();
        if(!std::filesystem::create_directory(argv[2]))throw std::runtime_error("New epoch capture directory required");
        MacRegisterBus bus;
        archer::with_wlan_io_window(bus,[&]{
            if(archer::read_chip_identity(bus).analog_id!=0x6933)throw std::runtime_error("Unexpected analog identity");
            for(const auto [a,v]:std::vector<std::pair<unsigned,unsigned>>{{0xe0,3},{0x90,0x6f},{0x80,0xeb},{0x81,0xeb}})
                if(archer::read_xtal_power_byte(bus,a)!=v)throw std::runtime_error("Powered profile mismatch");
            const auto pll=archer::read_xtal_power_byte(bus,0xe1);
            if(pll!=0x52&&pll!=0xfa)throw std::runtime_error("Powered PLL mismatch");
            if(archer::bb_init_read(bus,0x10398)&0x3ffff)throw std::runtime_error("Network TX must be paused before epoch boundary");
            {
                MacBulkChannel usb; // Synchronous: no pending receive requests at reset.
                archer::reset_to_usb2_scc(bus,[](unsigned us){std::this_thread::sleep_for(std::chrono::microseconds(us));});
                std::cout<<"{\"phase\":\"epoch_mac_reset_verified\"}"<<std::endl;
                archer::enable_adda_power(bus);
                archer::boot_firmware(bus,images,[&](const auto& frame){usb.send(frame);},
                    [](const char* phase){std::cout<<"{\"phase\":\""<<phase<<"\"}"<<std::endl;},true);
            }
            // No radio-table or other H2C submission in this new firmware epoch.
            if((bus.read(0x1e0,4)&0x0c00003f)!=0x0c000022||(bus.read(0x84,4)&0x300007)!=2)
                throw std::runtime_error("Reloaded firmware controls do not match resume prerequisites");
            unsigned drained=0;
            {
                MacBulkChannel receive(true);
                const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(3);
                auto quiet_since=std::chrono::steady_clock::now();
                bool quiet=false;
                while(std::chrono::steady_clock::now()<limit){
                    for(unsigned pipe=0;pipe<2;++pipe){
                        auto bytes=receive.receive(pipe,10);
                        if(bytes.empty())continue;
                        if(drained>=1024)throw std::runtime_error("Epoch drain transfer limit exceeded");
                        std::ofstream out(std::filesystem::path(argv[2])/("drain-"+std::to_string(drained++)+".bin"),std::ios::binary);
                        out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());out.close();
                        if(!out)throw std::runtime_error("Cannot retain epoch drain");
                        quiet_since=std::chrono::steady_clock::now();
                    }
                    if(std::chrono::steady_clock::now()-quiet_since>=std::chrono::milliseconds(250)){quiet=true;break;}
                }
                if(!quiet)throw std::runtime_error("Epoch transport did not quiesce; no ledger transition");
            } // All callbacks aborted and drained before reporting the boundary.
            std::cout<<"{\"result\":\"epoch_firmware_reloaded_no_h2c\",\"discarded_transfers\":"<<drained
                     <<",\"quiet_ms_min\":250,\"h2c_submissions\":0,\"resident_version_readback\":false}"<<std::endl;
        });
        return 0;
    }catch(const std::exception& e){std::cerr<<"epoch-boot: "<<e.what()<<"; keep transition fenced, no replay\n";return 2;}
}
