#pragma once
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
namespace archer {
// Updated native tools share the workflow lock; a coordinator owns it
// exclusively and passes its open-file description to each child process.
class WorkflowLock {
    int fd_=-1;
public:
    explicit WorkflowLock(const std::string& path){
        const int local=open(path.c_str(),O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600);
        if(local<0)throw std::runtime_error("Cannot open workflow lock");
        struct stat target{};
        if(fstat(local,&target)||!S_ISREG(target.st_mode)||target.st_nlink!=1){close(local);throw std::runtime_error("Invalid workflow lock file");}
        if(const char* value=std::getenv("ARCHER_WORKFLOW_FD")){
            char* end=nullptr;errno=0;const long inherited=std::strtol(value,&end,10);
            struct stat source{};
            if(!*value||errno||*end||inherited<0||inherited>0x7fffffff||fstat(inherited,&source)||
               source.st_dev!=target.st_dev||source.st_ino!=target.st_ino||
               flock(inherited,LOCK_EX|LOCK_NB)){
                close(local);throw std::runtime_error("Invalid inherited workflow ownership");
            }
            fd_=fcntl(inherited,F_DUPFD_CLOEXEC,0);close(local);
            if(fd_<0)throw std::runtime_error("Cannot retain workflow ownership");
        }else{
            if(flock(local,LOCK_SH|LOCK_NB)){close(local);throw std::runtime_error("A coordinated workflow owns this adapter");}
            fd_=local;
        }
    }
    ~WorkflowLock(){if(fd_>=0)close(fd_);}
    WorkflowLock(const WorkflowLock&)=delete;
    WorkflowLock& operator=(const WorkflowLock&)=delete;
};
}
