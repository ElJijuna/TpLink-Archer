#include "archer/macos_register_bus.hpp"
#include "archer/sequence_ledger.hpp"
#include <IOKit/scsi/SCSITaskLib.h>
static bool realtek_parent(io_registry_entry_t child){
    IOObjectRetain(child);io_registry_entry_t cursor=child;
    for(unsigned depth=0;depth<16;++depth){
        if(mac_usb_number(cursor,CFSTR("idVendor"))==0x0bda && mac_usb_number(cursor,CFSTR("idProduct"))==0x1a2b){IOObjectRelease(cursor);return true;}
        io_registry_entry_t parent=0;auto status=IORegistryEntryGetParentEntry(cursor,kIOServicePlane,&parent);
        IOObjectRelease(cursor);if(status)return false;cursor=parent;
    }
    IOObjectRelease(cursor);return false;
}
int main(){
    MMCDeviceInterface** mmc=nullptr;SCSITaskDeviceInterface** device=nullptr;bool exclusive=false;
    try{
        archer::SequenceLedger lock("artifacts/37ad-0104-sequences.bin");
        io_iterator_t iterator=0;
        if(IOServiceGetMatchingServices(kIOMainPortDefault,IOServiceMatching("IOCDMedia"),&iterator))throw std::runtime_error("Cannot enumerate optical devices");
        io_service_t selected=0;unsigned count=0;
        while(auto service=IOIteratorNext(iterator)){
            if(realtek_parent(service)){
                io_registry_entry_t cursor=service;IOObjectRetain(cursor);
                for(unsigned depth=0;depth<12;++depth){
                    if(IOObjectConformsTo(cursor,"IOSCSIPeripheralDeviceType05")){
                        ++count;if(!selected){selected=cursor;cursor=0;}break;
                    }
                    io_registry_entry_t parent=0;auto status=IORegistryEntryGetParentEntry(cursor,kIOServicePlane,&parent);
                    IOObjectRelease(cursor);cursor=0;if(status)break;cursor=parent;
                }
                if(cursor)IOObjectRelease(cursor);
            }
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
        if(count!=1){if(selected)IOObjectRelease(selected);throw std::runtime_error("Expected one Realtek installer optical service");}
        IOCFPlugInInterface** plugin=nullptr;SInt32 score=0;
        auto status=IOCreatePlugInInterfaceForService(selected,kIOMMCDeviceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);
        IOObjectRelease(selected);
        std::cout<<"plugin_status="<<status<<std::endl;
        if(status||!plugin)throw std::runtime_error("MMC client unavailable");
        auto queried=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOMMCDeviceInterfaceID),reinterpret_cast<LPVOID*>(&mmc));
        IODestroyPlugInInterface(plugin);
        if(queried||!mmc)throw std::runtime_error("MMC interface unavailable");
        device=(*mmc)->GetSCSITaskDeviceInterface(mmc);
        if(!device)throw std::runtime_error("SCSI task interface unavailable");
        status=(*device)->ObtainExclusiveAccess(device);
        std::cout<<"scsi_exclusive_status="<<status<<std::endl;
        if(status)throw std::runtime_error("SCSI exclusive access unavailable");
        exclusive=true;
        for(bool eject:{false,true}){
            auto task=(*device)->CreateSCSITask(device);if(!task)throw std::runtime_error("Cannot create SCSI task");
            UInt8 cdb[6]={static_cast<UInt8>(eject?0x1b:0x1e),0,0,0,static_cast<UInt8>(eject?2:0),0};
            status=(*task)->SetCommandDescriptorBlock(task,cdb,6);
            if(!status)status=(*task)->SetTimeoutDuration(task,1000);
            SCSI_Sense_Data sense{};SCSITaskStatus result{};UInt64 transferred=0;
            if(!status)status=(*task)->ExecuteTaskSync(task,&sense,&result,&transferred);
            (*task)->Release(task);
            std::cout<<"eject="<<eject<<" status="<<status<<" scsi_status="<<unsigned(result)<<std::endl;
            if(status)throw std::runtime_error("SCSI command failed; no retry");
            if(eject && result!=kSCSITaskStatus_GOOD)throw std::runtime_error("SCSI eject rejected");
        }
        (*device)->ReleaseExclusiveAccess(device);(*mmc)->Release(mmc);return 0;
    }catch(const std::exception& e){
        if(device&&exclusive)(*device)->ReleaseExclusiveAccess(device);
        if(mmc)(*mmc)->Release(mmc);
        std::cerr<<"scsi-eject: "<<e.what()<<'\n';return 2;
    }
}
