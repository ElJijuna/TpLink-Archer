#include "archer/installer_media.hpp"
#include <cassert>
#include <iostream>
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}assert(caught);}
int main(){
    assert(archer::installer_disk_path("disk16",true,true,false)=="/dev/disk16");
    for(auto name:{"disk","rdisk16","disk16s1","disk../1","/dev/disk16"})
        rejects([&]{archer::installer_disk_path(name,true,true,false);});
    rejects([]{archer::installer_disk_path("disk16",false,true,false);});
    rejects([]{archer::installer_disk_path("disk16",true,false,false);});
    rejects([]{archer::installer_disk_path("disk16",true,true,true);});
    std::cout<<"Installer optical media path and attributes: OK\n";
}
