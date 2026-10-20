#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/efuse_reader.hpp"
#include <filesystem>
#include <fstream>
int main(int argc,char** argv) {
    const bool security=argc==3&&std::string(argv[1])=="--security-only";
    if((argc!=2&&!security) || (argc==2&&std::string(argv[1]).rfind("--",0)==0)) {
        std::cout << "Usage: archer-efuse [--security-only] NEW_DIRECTORY (run from project root)\n";
        return argc==2&&std::string(argv[1])=="--help" ? 0 : 2;
    }
    try {
        archer::SequenceLedger exclusive("artifacts/37ad-0104-sequences.bin");
        const std::filesystem::path directory(argv[security?2:1]);
        if(!std::filesystem::create_directory(directory)) throw std::runtime_error("Output directory must be new");
        MacRegisterBus bus;
        archer::EfuseMaps maps;
        if(security)archer::with_wlan_io_window(bus,[&] { maps=archer::read_ddv_efuse(bus,archer::EfuseProfile::firmware_security); });
        else (void)archer::query_phy_capabilities_with_io_window(bus,[&] { maps=archer::read_ddv_efuse(bus); });
        auto save=[&](const char* name,const std::vector<std::uint8_t>& bytes) {
            std::ofstream out(directory/name,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            out.close();
            if(!out) throw std::runtime_error("Cannot preserve efuse map");
        };
        if(security)save("firmware-security.bin",maps.security);
        else {save("physical-ddv.bin",maps.physical); save("phycap.bin",maps.phycap);}
        return 0;
    } catch(const std::exception& error) { std::cerr << "efuse: " << error.what() << '\n'; return 2; }
}
