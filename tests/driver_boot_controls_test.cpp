#include "archer/driver_boot_controls.hpp"
#include "boot-control-fixtures.hpp"
#include <cassert>
#include <iostream>
int main(){
 using archer::DriverBootControls;
 assert(archer::classify_driver_boot_controls(cold_fixture)==DriverBootControls::cold_controls_only);
 assert(archer::classify_driver_boot_controls(running_fixture)==DriverBootControls::running_controls_only);
 for(unsigned i=0;i<6;++i)for(auto sentinel:{0xeaeaeaeau,0xdeadbeefu}){
  auto v=cold_fixture;v[i]=sentinel;assert(archer::classify_driver_boot_controls(v)==DriverBootControls::inaccessible);
 }
 auto v=cold_fixture;v[0]^=1;assert(archer::classify_driver_boot_controls(v)==DriverBootControls::unsupported_chip_cut);
 v=cold_fixture;v[1]^=1<<12;assert(archer::classify_driver_boot_controls(v)==DriverBootControls::unsupported_chip_cut);
 v=cold_fixture;v[5]=1;assert(archer::classify_driver_boot_controls(v)==DriverBootControls::unknown);
 for(unsigned status:{4u,5u,6u,7u,15u}){v=cold_fixture;v[2]=(v[2]&~0x3c000000)|(status<<26);assert(archer::classify_driver_boot_controls(v)==DriverBootControls::unknown);}
 v=running_fixture;v[4]|=1<<17;assert(archer::classify_driver_boot_controls(v)==DriverBootControls::unknown);
 std::cout<<"Boot control observation: cold physical trace, synthetic running controls, sentinel/cut/ROM rejection passed; no reset permission implied\n";
}
