#include "archer/sequence_ledger.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}assert(caught);}
int main(){
    char pattern[]="/tmp/archer-workflow-XXXXXX";const auto tmp=mkdtemp(pattern);assert(tmp);
    const auto ledger=std::string(tmp)+"/ledger",path=ledger+".workflow";
    int owner=open(path.c_str(),O_RDWR|O_CREAT,0600);assert(owner>=0&&!flock(owner,LOCK_EX|LOCK_NB));
    rejects([&]{archer::SequenceLedger blocked(ledger);});
    // A separate inherited descriptor refers to the same ownership, not a
    // newly opened fd that merely names the same file.
    const auto id=std::to_string(owner);setenv("ARCHER_WORKFLOW_FD",id.c_str(),1);
    {archer::SequenceLedger child(ledger);assert(child.reserve()==2);}
    unsetenv("ARCHER_WORKFLOW_FD");
    rejects([&]{archer::SequenceLedger still_blocked(ledger);});
    setenv("ARCHER_WORKFLOW_FD","bad",1);rejects([&]{archer::SequenceLedger bad(ledger);});
    const auto wrong=open((std::string(tmp)+"/other").c_str(),O_RDWR|O_CREAT,0600);assert(wrong>=0);
    setenv("ARCHER_WORKFLOW_FD",std::to_string(wrong).c_str(),1);rejects([&]{archer::SequenceLedger bad(ledger);});
    unsetenv("ARCHER_WORKFLOW_FD");close(wrong);close(owner);
    {archer::SequenceLedger released(ledger);assert(released.reserve()==3);}
    std::filesystem::remove_all(tmp);
    std::cout<<"Workflow ownership inheritance, exclusion and descriptor identity: OK\n";
}
