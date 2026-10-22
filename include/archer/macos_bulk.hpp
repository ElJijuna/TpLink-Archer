#pragma once
#include "archer/macos_register_bus.hpp"
#include <vector>
#include <array>
#include <deque>
#include <chrono>
// Single-threaded channel: create, receive and close on the owning run-loop thread.
// Async callbacks run only while that thread pumps the private run-loop mode.
class MacBulkChannel {
    IOUSBInterfaceInterface300** usb=nullptr;bool opened=false;UInt8 output=0;std::vector<UInt8> inputs,input_addresses;
    struct AsyncSlot {
        MacBulkChannel* owner=nullptr;unsigned index=0;bool pending=false;
        std::array<std::uint8_t,20480> bytes{};
    };
    std::array<AsyncSlot,9> slots{};
    std::array<std::deque<std::vector<std::uint8_t>>,2> completed;
    CFRunLoopSourceRef source=nullptr;
    bool async_mode=false,stopping=false;unsigned pending=0;IOReturn async_error=0;
    static CFStringRef mode(){return CFSTR("ArcherBulkReceive");}
    void submit(AsyncSlot& slot){
        auto status=(*usb)->ReadPipeAsyncTO(usb,inputs[slot.index],slot.bytes.data(),slot.bytes.size(),0,0,&completion,&slot);
        if(status){async_error=status;return;}
        slot.pending=true;++pending;
    }
    static void completion(void* refcon,IOReturn status,void* arg) noexcept {
        auto& slot=*static_cast<AsyncSlot*>(refcon);auto& self=*slot.owner;
        slot.pending=false;--self.pending;
        const auto size=reinterpret_cast<std::uintptr_t>(arg);
        try{
            std::cout<<"{\"operation\":\"bulk_async_completion\",\"endpoint\":"<<unsigned(self.input_addresses[slot.index])
                     <<",\"status\":"<<status<<",\"reported_size\":"<<size
                     <<",\"size_valid\":"<<(status==0?"true":"false")
                     <<",\"stopping\":"<<(self.stopping?"true":"false")<<"}"<<std::endl;
            if(self.stopping)return; // Aborted buffers are never decoded or resubmitted.
            if(status){self.async_error=status;return;}
            if(size>slot.bytes.size()||self.completed[slot.index].size()>=1024){self.async_error=kIOReturnOverrun;return;}
            if(size)self.completed[slot.index].emplace_back(slot.bytes.begin(),slot.bytes.begin()+size);
            self.submit(slot);
        }catch(...){self.async_error=kIOReturnNoMemory;}
    }
    void start_async(){
        auto status=(*usb)->CreateInterfaceAsyncEventSource(usb,&source);
        if(status||!source)throw std::runtime_error("Cannot create USB async event source");
        CFRunLoopAddSource(CFRunLoopGetCurrent(),source,mode());async_mode=true;
        // Match Linux's eight receive blocks on endpoint 0x84. Keep one on
        // 0x88 as a separately identified control-path diagnostic.
        for(unsigned n=0;n<slots.size();++n){
            auto& slot=slots[n];slot.owner=this;
            slot.index=n<8?(input_addresses[0]==0x84?0:1):(input_addresses[0]==0x88?0:1);
            submit(slot);if(async_error)throw std::runtime_error("Cannot queue USB async read");
        }
        std::cout<<"{\"operation\":\"bulk_async_started\",\"pending\":"<<pending<<",\"endpoint84_slots\":8,\"endpoint88_slots\":1,\"usb_timeout_ms\":0}"<<std::endl;
    }
public:
    MacBulkChannel(const MacBulkChannel&)=delete;
    MacBulkChannel& operator=(const MacBulkChannel&)=delete;
    explicit MacBulkChannel(bool persistent_async=false){
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
                if(dir==kUSBIn&&type==kUSBBulk&&packet==512&&(ep==4||ep==8)){inputs.push_back(pipe);input_addresses.push_back(ep|0x80);}
                if(dir==kUSBOut&&ep==7&&type==kUSBBulk&&packet==512){if(output)throw std::runtime_error("Ambiguous boot output");output=pipe;}
            }
            if(inputs.size()!=2)throw std::runtime_error("Expected two firmware input pipes");
            if(!output)throw std::runtime_error("USB2 endpoint 0x07 absent");
            if(persistent_async)start_async();
        }catch(...){close();throw;}
    }
    void close(){if(usb){
        if(async_mode){
            stopping=true;
            for(auto pipe:inputs){
                const auto status=(*usb)->AbortPipe(usb,pipe);
                std::cout<<"{\"operation\":\"bulk_async_abort\",\"pipe\":"<<unsigned(pipe)<<",\"status\":"<<status<<"}"<<std::endl;
            }
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(pending && std::chrono::steady_clock::now()<deadline)CFRunLoopRunInMode(mode(),0.01,true);
            // Never release callback storage while IOKit may still reference it.
            if(pending){std::cerr<<"USB callbacks did not drain; terminating to preserve buffer lifetime\n";std::terminate();}
            std::cout<<"{\"operation\":\"bulk_async_drained\",\"pending\":0}"<<std::endl;
            async_mode=false;
        }
        if(source){CFRunLoopRemoveSource(CFRunLoopGetCurrent(),source,mode());CFRelease(source);source=nullptr;}
        if(opened)(*usb)->USBInterfaceClose(usb);(*usb)->Release(usb);usb=nullptr;}}
    ~MacBulkChannel(){close();}
    void send(const std::vector<std::uint8_t>& frame){
        auto status=(*usb)->WritePipeTO(usb,output,const_cast<std::uint8_t*>(frame.data()),frame.size(),500,1000);
        std::cout<<"{\"operation\":\"firmware_bulk_write\",\"bytes\":"<<frame.size()<<",\"status\":"<<status<<"}"<<std::endl;
        if(status)throw std::runtime_error("Firmware write failed; no retry");
    }
    std::vector<std::uint8_t> receive(unsigned index,unsigned timeout_ms=20){
        if(index>=inputs.size()||!timeout_ms||timeout_ms>1000)throw std::invalid_argument("Invalid bulk receive bound");
        if(async_mode){
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout_ms);
            while(completed[index].empty()&&!async_error&&std::chrono::steady_clock::now()<deadline){
                const auto seconds=std::chrono::duration<double>(deadline-std::chrono::steady_clock::now()).count();
                CFRunLoopRunInMode(mode(),seconds>0?seconds:0,true);
            }
            if(async_error)throw std::runtime_error("Persistent USB receive failed; no retry");
            if(completed[index].empty())return {};
            auto bytes=std::move(completed[index].front());completed[index].pop_front();return bytes;
        }
        std::vector<std::uint8_t> bytes(20480);UInt32 size=bytes.size();
        auto status=(*usb)->ReadPipeTO(usb,inputs[index],bytes.data(),&size,timeout_ms,timeout_ms);
        const bool timeout=status==kIOUSBTransactionTimeout||status==kIOReturnTimeout;
        std::cout<<"{\"operation\":\"bulk_receive_result\",\"input_index\":"<<index
                 <<",\"pipe\":"<<unsigned(inputs[index])<<",\"endpoint\":"<<unsigned(input_addresses[index])
                 <<",\"requested_bytes\":"<<bytes.size()<<",\"reported_size\":"<<size
                 <<",\"size_valid\":"<<(status==0?"true":"false")<<",\"timeout\":"<<(timeout?"true":"false")
                 <<",\"status\":"<<status<<"}"<<std::endl;
        // SDK ReadPipeTO contract: on error size is unchanged and buffer invalid.
        // Never reinterpret timeout contents as a partial, valid USB transfer.
        if(timeout)return {};
        if(status||size>bytes.size())throw std::runtime_error("Firmware receive failed");
        bytes.resize(size);return bytes;
    }

};
