#pragma once
#include "archer/register_mailbox.hpp"
#include <map>
#include <vector>
namespace archer {
inline void with_bb_table_window(RegisterBus& bus,const std::vector<std::pair<unsigned,unsigned>>& table,const std::function<void()>& inspect){
    if(table.empty()||table.size()>1024)throw std::runtime_error("Invalid BB table size");
    std::map<unsigned,unsigned> saved;std::vector<unsigned> changed;
    for(auto [a,v]:table){(void)v;if(a<0x20000||a>0x2ffff||a%4)throw std::runtime_error("Invalid BB table address");
        if(!saved.count(a)){auto old=bus.read(a,4);if(old==0xdeadbeef||old==0xeaeaeaea)throw std::runtime_error("BB table baseline inaccessible at "+std::to_string(a));saved[a]=old;}}
    std::exception_ptr failure;
    try{for(auto [a,v]:table){changed.push_back(a);bus.write(a,4,v);}bus.pause();inspect();}catch(...){failure=std::current_exception();}
    std::string errors;std::map<unsigned,bool> restored;
    for(auto i=changed.rbegin();i!=changed.rend();++i){auto a=*i;if(restored[a])continue;restored[a]=true;
        try{bus.write(a,4,saved.at(a));if(bus.read(a,4)!=saved.at(a))throw std::runtime_error("BB table restore mismatch at "+std::to_string(a));}catch(const std::exception& e){errors+=std::string(e.what())+"; ";}}
    if(!errors.empty())throw std::runtime_error("BB table restoration failed: "+errors);
    if(failure)std::rethrow_exception(failure);
}
}
