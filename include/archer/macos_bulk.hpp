#pragma once
#include "archer/macos_register_bus.hpp"
#include <vector>
class MacBulkChannel {
    IOUSBInterfaceInterface300** usb=nullptr;bool opened=false;UInt8 output=0;std::vector<UInt8> inputs;
public:
    MacBulkChannel(){
        io_iterator_t iterator=0;
        if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("IOUSBHostInterface"),&iterator))throw std::runtime_error("Cannot enumerate boot USB");
        io_service_t selected=0;unsigned matches=0;
        while(auto service=IOIteratorNext(iterator)){
            if(mac_usb_number(service,CFSTR("idVendor"))==0x37ad&&mac_usb_number(service,CFSTR("idProduct"))==0x0104&&mac_usb_number(service,CFSTR("bInterfaceNumber"))==0){
                ++matches;if(!selected){selected=service;continue;}
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
        if(matches!=1){if(selected)IOObjectRelease(selected);throw std::runtime_error("Expected one boot interface");}
        IOCFPlugInInterface** plugin=nullptr;SInt32 score=0;
        auto status=IOCreatePlugInInterfaceForService(selected,kIOUSBInterfaceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);
        IOObjectRelease(selected);
        if(status||!plugin)throw std::runtime_error("Cannot create boot USB client");
        auto q=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),reinterpret_cast<LPVOID*>(&usb));
        IODestroyPlugInInterface(plugin);
        if(q||!usb)throw std::runtime_error("Boot interface unavailable");
        try{
            status=(*usb)->USBInterfaceOpen(usb);if(status)throw std::runtime_error("Cannot acquire boot USB interface");opened=true;
            UInt8 count=0;if((*usb)->GetNumEndpoints(usb,&count)||count!=9)throw std::runtime_error("Unexpected boot endpoints");
            for(UInt8 pipe=1;pipe<=count;++pipe){
                UInt8 dir=0,ep=0,type=0,interval=0;UInt16 packet=0;
                if((*usb)->GetPipeProperties(usb,pipe,&dir,&ep,&type,&packet,&interval))throw std::runtime_error("Cannot inspect boot pipes");
                if(dir==kUSBIn&&type==kUSBBulk&&packet==512&&(ep==4||ep==8))inputs.push_back(pipe);
                if(dir==kUSBOut&&ep==7&&type==kUSBBulk&&packet==512){if(output)throw std::runtime_error("Ambiguous boot output");output=pipe;}
            }
            if(inputs.size()!=2)throw std::runtime_error("Expected two firmware input pipes");
            if(!output)throw std::runtime_error("USB2 endpoint 0x07 absent");
        }catch(...){close();throw;}
    }
    void close(){if(usb){if(opened)(*usb)->USBInterfaceClose(usb);(*usb)->Release(usb);usb=nullptr;}}
    ~MacBulkChannel(){close();}
    void send(const std::vector<std::uint8_t>& frame){
        auto status=(*usb)->WritePipeTO(usb,output,const_cast<std::uint8_t*>(frame.data()),frame.size(),500,1000);
        std::cout<<"{\"operation\":\"firmware_bulk_write\",\"bytes\":"<<frame.size()<<",\"status\":"<<status<<"}"<<std::endl;
        if(status)throw std::runtime_error("Firmware write failed; no retry");
    }
    std::vector<std::uint8_t> receive(unsigned index,unsigned timeout_ms=20){
        if(index>=inputs.size()||!timeout_ms||timeout_ms>1000)throw std::invalid_argument("Invalid bulk receive bound");
        std::vector<std::uint8_t> bytes(20480);UInt32 size=bytes.size();
        auto status=(*usb)->ReadPipeTO(usb,inputs[index],bytes.data(),&size,timeout_ms,timeout_ms);
        if(status==kIOUSBTransactionTimeout||status==kIOReturnTimeout)return {};
        if(status||size>bytes.size())throw std::runtime_error("Firmware receive failed");
        bytes.resize(size);return bytes;
    }

};
