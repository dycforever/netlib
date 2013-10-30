#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "util.h"

namespace dyc
{

///
/// Wrapper of ::sockaddr_in.
///
/// This is an POD interface class.
class InetAddress {
 public:
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  explicit InetAddress(uint16_t port);
  InetAddress(){}

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(const std::string& ip, uint16_t port);

  /// Constructs an endpoint with given struct @c ::sockaddr_in
  /// Mostly used when accepting new connections
  InetAddress(const struct ::sockaddr_in& addr)
    : m_addr(addr)
  { }

  void toIp(char* buf, size_t size,
                   const struct ::sockaddr_in& addr) const;

  std::string toIpPort() const;
  std::string toHostPort() const __attribute__ ((deprecated))
  { return toIpPort(); }

  // default copy/assignment are Okay

  const struct ::sockaddr_in& getSockAddrInet() const { return m_addr; }
  void setSockAddrInet(const struct ::sockaddr_in& addr) { m_addr = addr; }
  void setSockAddrInet(const std::string& ip, uint16_t port);
  void setSockAddrInet(const char* ip, uint16_t port);

  uint32_t getRowIp() const { return m_addr.sin_addr.s_addr; }
  uint16_t getRowPort() const { return m_addr.sin_port; }

 private:
  struct ::sockaddr_in m_addr;
};

}

#endif  // MUDUO_NET_INETADDRESS_H
