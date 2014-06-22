
#ifndef NETLIB_UTIL_H
#define NETLIB_UTIL_H

#include <netinet/in.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>




#include <iostream>
#include <stdint.h>
#include <endian.h>

#include <fcntl.h>
#include <stdio.h>  // snprintf
#include <strings.h>  // bzero
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>          /* See NOTES */

#include "log.h"


namespace dyc {

typedef struct sockaddr SA;

extern int error;

 inline const SA* sockaddr_cast(const struct ::sockaddr_in* addr)
{
    SA* tmp = reinterpret_cast<SA*>(const_cast<struct ::sockaddr_in*>(addr));
    return tmp; 
}

 inline SA* sockaddr_cast(struct ::sockaddr_in* addr)
{
    SA* tmp = reinterpret_cast<SA*>(addr);
    return tmp;
}

 inline void setNonBlockAndCloseOnExec(int sockfd)
{
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}

inline int createICMPSocket()
{
  // socket
  int sockfd = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0)
  {
    FATAL("createDGramSocket failed, errno:%d with %s", errno, strerror(errno));
    return -1;
  }

  return sockfd;
}

inline int createDGramSocket()
{
  // socket
  int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    FATAL("createDGramSocket failed, errno:%d with %s", errno, strerror(errno));
    return -1;
  }

  return sockfd;
}


inline void fromIpPort(const char* ip, uint16_t port,
                           struct ::sockaddr_in* addr)
{
  addr->sin_family = AF_INET;
  addr->sin_port = port;
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
  {
    FATAL("fromIpPort failed");
  }
}

inline void fromIpPort(const std::string ip, uint16_t port,
                           struct ::sockaddr_in* addr)
{
    fromIpPort(ip.c_str(), port, addr);
}

// inline int getSocketError(int sockfd)
// {
//   int optval;
//   socklen_t optlen = static_cast<socklen_t>(sizeof optval);
// 
//   if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
//     return errno;
//   } else {
//     return optval;
//   }
// }

inline struct ::sockaddr_in getLocalAddr(int sockfd)
{
  struct ::sockaddr_in localaddr;
  bzero(&localaddr, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    std::cerr << "getLocalAddr";
  }
  return localaddr;
}

inline struct ::sockaddr_in getPeerAddr(int sockfd)
{
  struct ::sockaddr_in peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
  {
    std::cerr << "getPeerAddr";
  }
  return peeraddr;
}

inline bool isSelfConnect(int sockfd)
{
  struct ::sockaddr_in localaddr = getLocalAddr(sockfd);
  struct ::sockaddr_in peeraddr = getPeerAddr(sockfd);
  return localaddr.sin_port == peeraddr.sin_port
      && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

}

#endif  // NETLIB_UTIL_H
