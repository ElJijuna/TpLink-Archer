#include "archer/rx_delivery_audit.hpp"
#include <cassert>
#include <iostream>
struct Bus:archer::RegisterBus {
    unsigned reads=0,writes=0;int inaccessible=-1,fail=-1;
    unsigned read(unsigned address,unsigned width)override{
        const unsigned i=reads++;
        const auto& f=archer::rx_delivery_fields.at(i);
        assert(address==f.address&&width==f.width);
        if(int(i)==fail)throw std::runtime_error("USB register failure");
        if(int(i)==inaccessible)return 0xdeadbeef;
        return f.source_value;
    }
    void write(unsigned,unsigned,unsigned)override{++writes;throw std::runtime_error("Unexpected write");}
    void pause()override{throw std::runtime_error("Unexpected pause");}
};
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    Bus ok;const auto values=archer::read_rx_delivery_audit(ok);
    assert(values.size()==24&&ok.reads==24&&!ok.writes);
    for(unsigned i=0;i<values.size();++i){
        const auto& f=archer::rx_delivery_fields[i];
        assert((f.source_value&~f.mask)==0);
        assert(f.width==4||(f.width==2&&f.mask<=0xffff));
    }
    for(int index:{0,11,23}){
        Bus bad;bad.inaccessible=index;rejects([&]{archer::read_rx_delivery_audit(bad);});
        assert(bad.reads==unsigned(index+1)&&!bad.writes);
        Bus failed;failed.fail=index;rejects([&]{archer::read_rx_delivery_audit(failed);});
        assert(failed.reads==unsigned(index+1)&&!failed.writes);
    }
    std::cout<<"Read-only RX delivery audit bounds and fail-fast transport checks passed\n";
}
