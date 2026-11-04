#include "archer/credential_pipe.hpp"
#include <cassert>
#include <fcntl.h>
#include <iostream>
using namespace archer;
template<class F>void rejects(F f){bool bad=false;try{f();}catch(...){bad=true;}assert(bad);}
int input(const std::string& s){int fd[2];assert(pipe(fd)==0);assert(write(fd[1],s.data(),s.size())==static_cast<ssize_t>(s.size()));close(fd[1]);return fd[0];}
int main(){auto actual=pmk_from_credential_pipe(input("password"));auto expected=derive_wpa2_pmk("password","Especial");assert(actual.bytes==expected.bytes);rejects([]{pmk_from_credential_pipe(input("short"));});rejects([]{pmk_from_credential_pipe(input(std::string(64,'a')));});rejects([]{pmk_from_credential_pipe(0);});int fd=open("tests/fixtures/wpa2-test-m2.bin",O_RDONLY);rejects([&]{pmk_from_credential_pipe(fd);});close(fd);std::cout<<"Credential pipe validation, bounds and derivation passed\n";}
