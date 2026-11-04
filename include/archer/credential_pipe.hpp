#pragma once
#include "archer/wpa2_handshake.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
namespace archer {
// Anonymous inherited pipe only. No credential path, argument, environment value or log.
inline SecretBytes<32> pmk_from_credential_pipe(int fd){
 struct stat st{};if(fd<3||fstat(fd,&st)||!S_ISFIFO(st.st_mode))throw std::invalid_argument("Private credential pipe required");
 struct Close {int fd;~Close(){close(fd);}} close_fd{fd};
 SecretBytes<64> input;std::size_t length=0;
 for(;;){
  pollfd ready{fd,POLLIN,0};int status;
  do{status=poll(&ready,1,1000);}while(status<0&&errno==EINTR);
  if(status<=0||ready.revents&(POLLERR|POLLNVAL))throw std::runtime_error("Credential pipe unavailable");
  const auto n=read(fd,input.bytes.data()+length,input.bytes.size()-length);
  if(n<0){if(errno==EINTR)continue;throw std::runtime_error("Credential pipe read failed");}
  if(!n)break;
  length+=n;if(length>=input.bytes.size())throw std::invalid_argument("Credential extent rejected");
 }
 return derive_wpa2_pmk({reinterpret_cast<const char*>(input.bytes.data()),length},"Especial");
}
}
