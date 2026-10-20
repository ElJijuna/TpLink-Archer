#include "archer/rf_transport.hpp"
#include <map>
#include <vector>
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    std::map<unsigned,unsigned> r{{0xfc,0x371},{0x3f0,0x2200000},{0x74,0x901004fd},
        {0x84,3},{0x842c,0x7ff9},{0x22c24,0},{0x22adc,0},{0x2f3bc,0xa0012345}};
    std::vector<std::pair<unsigned,unsigned>> writes;
    bool respond=true,fail_data=false;
    unsigned read(unsigned a,unsigned) override{
        if(a==0x22c24 && fail_data && !writes.empty())throw std::runtime_error("Injected status failure");return r[a];
    }
    void write(unsigned a,unsigned,unsigned v) override{
        writes.emplace_back(a,v);r[a]=v;
        if(a==0x22adc && (v&4) && respond)r[0x22c24]=0x800abcde;
    }
    void pause() override{}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
int main(){
    Bus success;archer::BeRfTransport rf(success);
    assert(rf.read_full(0,0x55)==0xabcde && !(success.r[0x22adc]&3));
    assert(rf.read_full(1,0x100ef)==0x12345);
    rf.submit_full_write(1,0x100ef,0x54321);assert(success.r[0x2f3bc]==0xa0054321);
    rf.submit_full_write(0,0x55,0x12345);assert(success.r[0x22ae0]==0x01234555);
    for(auto address:{0x84u,0x842cu,0x74u,0xfcu}){
        Bus blocked;blocked.r[address]=0;archer::BeRfTransport port(blocked);
        rejects([&]{port.submit_full_write(0,0x55,1);});assert(blocked.writes.empty());
    }
    Bus isolated;isolated.r[0x84]=0x13;archer::BeRfTransport isolate(isolated);
    rejects([&]{isolate.read_full(0,1);});assert(isolated.writes.empty());
    Bus timeout;timeout.respond=false;archer::BeRfTransport no_reply(timeout);
    rejects([&]{no_reply.read_full(0,1);});assert(!(timeout.r[0x22adc]&3));
    Bus busy;busy.r[0x22c24]=1u<<29;archer::BeRfTransport wait(busy);
    rejects([&]{wait.read_full(0,1);});assert(busy.writes.empty());
    Bus sentinel;sentinel.r[0x22c24]=0xeaeaeaea;archer::BeRfTransport inaccessible(sentinel);
    rejects([&]{inaccessible.submit_full_write(0,1,0);});assert(sentinel.writes.empty());
    Bus failed;failed.fail_data=true;archer::BeRfTransport fault(failed);
    rejects([&]{fault.read_full(0,1);});assert(!(failed.r[0x22adc]&3));
    std::cout<<"RF prerequisites, bounded HWSI and cleanup: OK\n";
}
