#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include "archer/storage_eject.hpp"
int main(int argc,char** argv){
    const bool inspect=argc==2&&std::string(argv[1])=="--inspect";
    const bool recover=argc==2&&std::string(argv[1])=="--reset-bot-eject-installer";
    if(argc!=2||(!inspect&&!recover&&std::string(argv[1])!="--eject-installer")) {
        std::cout<<"Usage: archer-mode-switch --inspect | --eject-installer | --reset-bot-eject-installer\n";return 2;
    }
    IOUSBInterfaceInterface300** usb=nullptr;bool opened=false,unconfirmed=false;
    try {
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        io_iterator_t iterator=0;
        if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("IOUSBHostInterface"),&iterator)!=0)
            throw std::runtime_error("Cannot enumerate interfaces");
        io_service_t selected=0;unsigned matches=0;
        while(auto service=IOIteratorNext(iterator)){
            if(mac_usb_number(service,CFSTR("idVendor"))==0x0bda &&
               mac_usb_number(service,CFSTR("idProduct"))==0x1a2b &&
               mac_usb_number(service,CFSTR("bInterfaceNumber"))==0){
                ++matches;if(!selected){selected=service;continue;}
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
        if(matches!=1){if(selected)IOObjectRelease(selected);throw std::runtime_error("Expected exactly one Realtek installer interface");}
        IOCFPlugInInterface** plugin=nullptr;SInt32 score=0;
        auto status=IOCreatePlugInInterfaceForService(selected,kIOUSBInterfaceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);
        IOObjectRelease(selected);
        if(status||!plugin)throw std::runtime_error("Cannot create installer USB client");
        auto queried=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),reinterpret_cast<LPVOID*>(&usb));
        IODestroyPlugInInterface(plugin);
        if(queried||!usb)throw std::runtime_error("Cannot query installer interface");
        UInt8 cls=0,sub=0,protocol=0;
        if((*usb)->GetInterfaceClass(usb,&cls)||(*usb)->GetInterfaceSubClass(usb,&sub)||
           (*usb)->GetInterfaceProtocol(usb,&protocol)||cls!=8||sub!=6||protocol!=0x50)
            throw std::runtime_error("Expected SCSI bulk-only storage interface");
        if(inspect){
            io_service_t raw_device=0;
            if((*usb)->GetDevice(usb,&raw_device)||!raw_device)throw std::runtime_error("Cannot locate installer USB device");
            IOCFPlugInInterface** device_plugin=nullptr;SInt32 device_score=0;
            status=IOCreatePlugInInterfaceForService(raw_device,kIOUSBDeviceUserClientTypeID,kIOCFPlugInInterfaceID,&device_plugin,&device_score);
            IOObjectRelease(raw_device);
            if(status||!device_plugin)throw std::runtime_error("Cannot inspect installer configuration");
            IOUSBDeviceInterface197** device=nullptr;
            auto query=(*device_plugin)->QueryInterface(device_plugin,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID197),reinterpret_cast<LPVOID*>(&device));
            IODestroyPlugInInterface(device_plugin);
            if(query||!device)throw std::runtime_error("Device configuration client unavailable");
            IOUSBConfigurationDescriptorPtr config=nullptr;
            status=(*device)->GetConfigurationDescriptorPtr(device,0,&config);
            if(status||!config){(*device)->Release(device);throw std::runtime_error("Cannot read configuration descriptor");}
            const auto length=USBToHostWord(config->wTotalLength);
            if(length<9||length>4096){(*device)->Release(device);throw std::runtime_error("Invalid configuration size");}
            const auto bytes=reinterpret_cast<const unsigned char*>(config);
            std::cout<<"{\"configuration_hex\":\"";
            const char* hex="0123456789abcdef";
            for(unsigned i=0;i<length;++i)std::cout<<hex[bytes[i]>>4]<<hex[bytes[i]&15];
            std::cout<<"\"}"<<std::endl;
            (*device)->Release(device);(*usb)->Release(usb);return 0;
        }
        status=(*usb)->USBInterfaceOpen(usb);
        std::cout<<"{\"operation\":\"interface_open\",\"status\":"<<status<<"}"<<std::endl;
        if(status){
            status=(*usb)->USBInterfaceOpenSeize(usb);
            std::cout<<"{\"operation\":\"installer_interface_seize\",\"status\":"<<status<<"}"<<std::endl;
        }
        if(status)throw std::runtime_error("Cannot acquire installer storage interface");
        opened=true;

        UInt8 count=0,in=0,out=0;
        if((*usb)->GetNumEndpoints(usb,&count)||count!=2)throw std::runtime_error("Unexpected storage endpoints");
        for(UInt8 pipe=1;pipe<=count;++pipe){
            UInt8 direction=0,endpoint=0,type=0,interval=0;UInt16 packet=0;
            if((*usb)->GetPipeProperties(usb,pipe,&direction,&endpoint,&type,&packet,&interval)||type!=kUSBBulk||packet!=512)
                throw std::runtime_error("Unexpected USB2 storage pipe");
            std::cout<<"{\"pipe\":"<<unsigned(pipe)<<",\"endpoint\":"<<unsigned(endpoint|(direction==kUSBIn?0x80:0))<<"}"<<std::endl;
            if(direction==kUSBIn){if(in)throw std::runtime_error("Ambiguous IN pipe");in=pipe;}
            if(direction==kUSBOut){if(out)throw std::runtime_error("Ambiguous OUT pipe");out=pipe;}
        }
        if(!in||!out)throw std::runtime_error("Expected one bulk IN and one bulk OUT");
        if(recover){
            IOUSBDevRequestTO reset{};
            reset.bmRequestType=0x21;reset.bRequest=0xff;
            reset.wValue=0;reset.wIndex=0;reset.wLength=0;
            reset.noDataTimeout=500;reset.completionTimeout=1000;
            status=(*usb)->ControlRequestTO(usb,0,&reset);
            std::cout<<"{\"operation\":\"bot_reset\",\"status\":"<<status<<"}"<<std::endl;
            if(status)throw std::runtime_error("Bulk-only reset failed; no command sent");
            for(UInt8 pipe:{in,out}){
                status=(*usb)->ClearPipeStallBothEnds(usb,pipe);
                std::cout<<"{\"operation\":\"clear_halt\",\"pipe\":"<<unsigned(pipe)<<",\"status\":"<<status<<"}"<<std::endl;
                if(status)throw std::runtime_error("Bulk-only halt recovery failed; no command sent");
            }
        }
        for(bool eject:{false,true}){
            auto cbw=archer::installer_eject_cbw(eject);
            status=(*usb)->WritePipeTO(usb,out,cbw.data(),cbw.size(),500,1000);
            std::cout<<"{\"operation\":\""<<(eject?"eject":"allow_removal")<<"\",\"status\":"<<status<<"}"<<std::endl;
            if(status)throw std::runtime_error("Storage command submission failed; no retry");
            std::array<std::uint8_t,512> response{};UInt32 received=response.size();
            status=(*usb)->ReadPipeTO(usb,in,response.data(),&received,500,1000);
            std::cout<<"{\"operation\":\"csw\",\"status\":"<<status<<",\"requested_bytes\":512,\"bytes\":"<<(status?0:received)<<"}"<<std::endl;
            if(status){if(eject){unconfirmed=true;break;}throw std::runtime_error("Allow-removal response failed");}
            if(received>response.size())throw std::runtime_error("Invalid storage response length");
            std::cout<<"{\"csw_packet_hex\":\"";
            const char* hex="0123456789abcdef";
            for(unsigned i=0;i<received;++i)std::cout<<hex[response[i]>>4]<<hex[response[i]&15];
            std::cout<<"\"}"<<std::endl;
            auto code=archer::installer_csw_packet(response.data(),received,eject);
            std::cout<<"{\"csw_code\":"<<code<<"}"<<std::endl;
            if(eject&&code)throw std::runtime_error("Device rejected eject");
        }
        (*usb)->USBInterfaceClose(usb);(*usb)->Release(usb);return unconfirmed?3:0;
    }catch(const std::exception& e){
        if(usb){if(opened)(*usb)->USBInterfaceClose(usb);(*usb)->Release(usb);}
        std::cerr<<"mode-switch: "<<e.what()<<'\n';return 2;
    }
}
