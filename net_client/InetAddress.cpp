
#include <strings.h>  
#include "InetAddress.h"
#include <netinet/in.h>

namespace dyc {

// the inline assembler code makes type blur,
// so we disable warnings for a while.

InetAddress::InetAddress(uint16_t port)
{
  bzero(&m_addr, sizeof m_addr);
  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
  bzero(&m_addr, sizeof m_addr);
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons(port);
  inet_aton(ip.c_str(), &m_addr.sin_addr);
}

void InetAddress::setSockAddrInet(const char* ip, uint16_t port)
{
  bzero(&m_addr, sizeof m_addr);
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons(port);
  inet_aton(ip, &m_addr.sin_addr);
}

void InetAddress::setSockAddrInet(const std::string& ip, uint16_t port)
{
    setSockAddrInet(ip.c_str(), port);
}

void InetAddress::toIp(char* buf, size_t size,
                   const struct sockaddr_in& addr) const {
//  assert(size >= INET_ADDRSTRLEN);
  ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

std::string InetAddress::toIpPort() const
{
    char buf[32];
    char host[INET_ADDRSTRLEN] = "INVALID";
    toIp(host, sizeof(host), m_addr);
    uint16_t port = m_addr.sin_port;
    snprintf(buf, 32, "%s:%u", host, port);
    return buf;
}

}
