#pragma once
#include <string>
#include <stdexcept>
namespace archer {
inline std::string installer_disk_path(const std::string& bsd,bool whole,bool removable,bool writable) {
    if(!whole||!removable||writable||bsd.size()<5||bsd.size()>16||bsd.substr(0,4)!="disk")
        throw std::invalid_argument("Expected whole removable read-only optical disk");
    for(unsigned i=4;i<bsd.size();++i)
        if(bsd[i]<'0'||bsd[i]>'9')throw std::invalid_argument("Invalid BSD disk identifier");
    return "/dev/"+bsd;
}
}
