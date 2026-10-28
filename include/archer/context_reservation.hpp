#pragma once
#include "archer/sequence_ledger.hpp"
class ContextReservation {
 int fd=-1;
public:
 ContextReservation(){
    fd=open("artifacts/37ad-0104-context-reservation",O_RDWR|O_CREAT|O_NOFOLLOW,0600);
    if(fd<0)throw std::runtime_error("Cannot open context reservation");
    struct stat st{};char state[6]{};
    if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||flock(fd,LOCK_EX|LOCK_NB)||
       (st.st_size && (st.st_size!=6||pread(fd,state,6,0)!=6||std::string(state,6)!="CLEAN\n"))){
        close(fd);fd=-1;throw std::runtime_error("Context reservation dirty/owned; recover firmware before reuse");
    }
    try{write_state("DIRTY\n");}catch(...){close(fd);fd=-1;throw;}
 }
 void write_state(const char* state){if(pwrite(fd,state,6,0)!=6||ftruncate(fd,6)||fsync(fd))throw std::runtime_error("Context reservation persistence failed");}
 void clean(){write_state("CLEAN\n");}
 ~ContextReservation(){if(fd>=0)close(fd);}
};
