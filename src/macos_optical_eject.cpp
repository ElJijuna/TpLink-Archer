#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/installer_media.hpp"
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
static bool realtek_ancestor(io_registry_entry_t entry){
    IOObjectRetain(entry);
    for(unsigned depth=0;depth<20;++depth){
        if(IOObjectConformsTo(entry,"IOUSBHostDevice") &&
           mac_usb_number(entry,CFSTR("idVendor"))==0x0bda &&
           mac_usb_number(entry,CFSTR("idProduct"))==0x1a2b){IOObjectRelease(entry);return true;}
        io_registry_entry_t parent=0;
        const auto status=IORegistryEntryGetParentEntry(entry,kIOServicePlane,&parent);
        IOObjectRelease(entry);if(status)return false;entry=parent;
    }
    IOObjectRelease(entry);return false;
}
static bool boolean(io_registry_entry_t media,CFStringRef key){
    const auto value=IORegistryEntryCreateCFProperty(media,key,kCFAllocatorDefault,0);
    if(!value || CFGetTypeID(value)!=CFBooleanGetTypeID()){if(value)CFRelease(value);throw std::runtime_error("Missing optical media attribute");}
    const bool result=CFBooleanGetValue(static_cast<CFBooleanRef>(value));
    CFRelease(value);return result;
}
int main(int argc,char** argv){
    const bool eject=argc==2&&std::string(argv[1])=="--eject-installer";
    if(argc!=2 || (!eject&&std::string(argv[1])!="--inspect")){
        std::cout<<"Usage: archer-optical-eject --inspect | --eject-installer\n";return 2;
    }
    io_service_t media=0;int fd=-1;
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        io_iterator_t iterator=0;
        if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("IOCDMedia"),&iterator))
            throw std::runtime_error("Cannot enumerate optical media");
        unsigned matches=0;
        while(auto candidate=IOIteratorNext(iterator)){
            io_name_t name{};
            if(!IORegistryEntryGetName(candidate,name)&&std::string(name)=="RTK Driver Storage Media"&&realtek_ancestor(candidate)){
                ++matches;if(!media){media=candidate;continue;}
            }
            IOObjectRelease(candidate);
        }
        IOObjectRelease(iterator);
        if(matches!=1)throw std::runtime_error("Expected exactly one Realtek installer CD");
        const auto name=IORegistryEntryCreateCFProperty(media,CFSTR("BSD Name"),kCFAllocatorDefault,0);
        char bsd[64]{};
        const bool named=name&&CFGetTypeID(name)==CFStringGetTypeID()&&CFStringGetCString(static_cast<CFStringRef>(name),bsd,sizeof(bsd),kCFStringEncodingUTF8);
        if(name)CFRelease(name);
        if(!named)throw std::runtime_error("Installer CD has no BSD identifier");
        const auto path=archer::installer_disk_path(bsd,boolean(media,CFSTR("Whole")),boolean(media,CFSTR("Removable")),boolean(media,CFSTR("Writable")));
        const auto expected_major=mac_usb_number(media,CFSTR("BSD Major")),expected_minor=mac_usb_number(media,CFSTR("BSD Minor"));
        if(expected_major<0||expected_minor<0)throw std::runtime_error("Missing BSD device numbers");
        std::cout<<"{\"verified_installer\":\""<<path<<"\",\"major\":"<<expected_major<<",\"minor\":"<<expected_minor<<"}"<<std::endl;
        if(!eject){IOObjectRelease(media);media=0;return 0;}
        if(geteuid()!=0)throw std::runtime_error("Administrator authentication required; run this tool with sudo in Terminal");
        fd=open(path.c_str(),O_RDONLY|O_NONBLOCK|O_NOFOLLOW);
        if(fd<0)throw std::runtime_error(std::string("Cannot open installer CD: ")+strerror(errno));
        struct stat info{};
        if(fstat(fd,&info)||!S_ISBLK(info.st_mode)||major(info.st_rdev)!=expected_major||minor(info.st_rdev)!=expected_minor||!realtek_ancestor(media))
            throw std::runtime_error("Optical device identity changed; refusing eject");
        const int status=ioctl(fd,DKIOCEJECT,0);
        const int error=status?errno:0;
        std::cout<<"{\"operation\":\"DKIOCEJECT\",\"status\":"<<status<<",\"errno\":"<<error<<"}"<<std::endl;
        close(fd);fd=-1;IOObjectRelease(media);media=0;
        if(status)throw std::runtime_error(std::string("Installer eject failed: ")+strerror(error));
        std::cout<<"Eject accepted; verify Wi-Fi USB 37ad:0104 before using register tools.\n";
        return 0;
    }catch(const std::exception& e){if(fd>=0)close(fd);if(media)IOObjectRelease(media);std::cerr<<"optical-eject: "<<e.what()<<'\n';return 2;}
}
