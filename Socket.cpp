
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero

#include "InetAddress.h"
#include "Socket.h"  
#include "netutils/Log.h"  

namespace dyc {

ssize_t Socket::sendmsg(const struct msghdr *msg, int flags) {
    return ::sendmsg(mSockfd, msg, flags);
}

ssize_t Socket::recvmsg(struct msghdr *msg, int flags) {
    return ::recvmsg(mSockfd, msg, flags);
}

int Socket::close() {
    int ret = ::close(mSockfd);
//    int ret  = 0;
    if (ret == 0) {
        INFO_LOG("close %d success", mSockfd);
    } else {
        FATAL_LOG("close %d failed: %d %s", mSockfd, errno, strerror(errno));
    }
    return ret;
}

Socket::~Socket()
{
//    if (::close(mSockfd) == 0) {
//        INFO_LOG("close %d success", mSockfd);
//    } else {
//        FATAL_LOG("close %d failed: %d %s", mSockfd, errno, strerror(errno));
//    }
}

int Socket::getopt(int level, int optname, void* optval, void* len) {
    int ret = ::getsockopt(mSockfd, level, optname, optval, (socklen_t*)len);
    if (ret < 0) {
        FATAL_LOG("getsockopt() failed, ret:%d socket[%d] level[%d] optname[%u] errno[%d] with %s", 
                ret, mSockfd, level, optname, errno, strerror(errno));
        return ret;
    }
    return ret;
}

bool Socket::checkConnected() {
    int sret = 0;
    int sretlen = static_cast<socklen_t>(sizeof sret);
    int ret = getopt(SOL_SOCKET, SO_ERROR, (void*)&sret, &sretlen);
    if(ret == -1)  {  
        FATAL_LOG("%s:%d, connection failed with errno: %d %s", __FILE__, __LINE__, errno, strerror(errno));  
        return false;  
    } else if(sret) {  
        FATAL_LOG("%s:%d, connection failed with errno: %d %s", __FILE__, __LINE__, errno, strerror(errno));  
        return false;  
    }
    return true;
}

int Socket::setopt(int level, int optname, void* optval, socklen_t len) {
    int ret = ::setsockopt(mSockfd, level, optname, optval, len);
    if (ret < 0) {
        FATAL_LOG("setsockopt() failed, ret:%d socket[%d] level[%d] optname[%u] errno[%d] with %s", 
                ret, mSockfd, level, optname, errno, strerror(errno));
    }
    return ret;
}

int Socket::connect(const InetAddress& peerAddr) {
   const struct sockaddr_in& sockAddr = peerAddr.getSockAddrInet();
   DEBUG_LOG("socket[%d] connect to addr[%s]", mSockfd, peerAddr.toIpPort().c_str());
   int ret = ::connect(mSockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
   if ((mBlocking && ret < 0) || 
           (!mBlocking && ret < 0 && errno != EINPROGRESS)) {
        FATAL_LOG("ret:%d connect socket[%d] to raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
        return ret;
   }

   return 0;
}

int Socket::bind(const InetAddress& addr)
{
    struct sockaddr_in sockAddr = addr.getSockAddrInet();
    setReuseAddr(true);
    int ret = ::bind(mSockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
    if (ret < 0)
    {
        FATAL_LOG("ret:%d bind socket[%d] raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
        return ret;
    }
    INFO_LOG("bind with socket[%d] raw_ip[%s] port[%u]", 
           mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
    return 0;
}

int Socket::listen(int backlog)
{
    int ret = ::listen(mSockfd, backlog);
    if (ret < 0) {
        FATAL_LOG("listen  Die errno:%d with %s", errno, strerror(errno));
        return ret;
    }
    return 0;
}

int Socket::accept(int sockfd, struct sockaddr_in* addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd = ::accept(sockfd, sockaddr_cast(addr),
            &addrlen);

    if (connfd < 0) {
        int savedErrno = errno;
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                FATAL_LOG("unexpected error of ::accept %d", savedErrno);
                break;
            default:
                FATAL_LOG("unknown error of ::accept %d", savedErrno);
                break;
        }
    }
    return connfd;
}

int Socket::accept(InetAddress& peeraddr)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int connfd = accept(mSockfd, &addr);
    if (connfd >= 0) {
        peeraddr.setSockAddrInet(addr);
    }
    return connfd;
}

int Socket::shutdownWrite()
{
    return shutdown(mSockfd, SHUT_WR);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(mSockfd, IPPROTO_TCP, TCP_NODELAY,
            &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(mSockfd, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        FATAL_LOG("SO_REUSEPORT failed.");
    }
}

bool Socket::getLocalAddr(InetAddress& addr) {
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(mSockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
        FATAL_LOG("sockets::getLocalAddr failed");
        return false;
    }
    addr = InetAddress(localaddr);
    return true;
}

bool Socket::getPeerAddr(InetAddress& addr)
{
  struct sockaddr_in peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(mSockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
  {
    FATAL_LOG("sockets::getPeerAddr failed");
    return false;
  }
  addr = InetAddress(peeraddr);
  return true;
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(mSockfd, SOL_SOCKET, SO_KEEPALIVE,
            &optval, static_cast<socklen_t>(sizeof optval));
}

int Socket::setLinger(bool on, int timeout) {
    int optval = on ? 1 : 0;
    struct linger lingerVal = {optval, timeout};
    ::setsockopt(mSockfd, SOL_SOCKET, SO_LINGER, (char *) &lingerVal, sizeof(lingerVal));
}

int Socket::setSendBuf(int val) {
    return setsockopt(mSockfd, SOL_SOCKET, SO_SNDBUF, (char *)&val, sizeof(val));
}

int Socket::getSendBuf(int* val, socklen_t* tmplen) {
	int ret = getsockopt(mSockfd, SOL_SOCKET, SO_SNDBUF, (char *)&val, tmplen);
    return ret;
}

int Socket::setRcvBuf(int val) {
    return setsockopt(mSockfd, SOL_SOCKET, SO_RCVBUF, (char *)&val, sizeof(val));
}

int Socket::getRcvBuf(int* val, socklen_t* tmplen) {
	int ret = getsockopt(mSockfd, SOL_SOCKET, SO_RCVBUF, (char *)&val, tmplen);
    return ret;
}

int Socket::send(const std::string& mesg) {
    return send(mesg.c_str(), mesg.size());
}

int Socket::send(const char* buf, size_t len) {
write_again:
    DEBUG_LOG("raw socket writing: %p", buf);
    int count = write(mSockfd, buf, len);
    if (count > 0) {
        return count;
    }
    switch(errno) {
        case EINTR:
            goto write_again;
        case EAGAIN:
            break;
        default:
            FATAL_LOG("write socket fd[%d] from[%p] return %d with errno[%d][%s], this socket is disconnected", 
                    mSockfd, buf, count, errno, strerror(errno));
    };
    return count;
}

long Socket::recv(char* buf, size_t len, int* errNo) {
    long count = 0;
read_again:
    count = ::read(mSockfd, buf, len);
    DEBUG_LOG("socket [%d] read %ld bytes", mSockfd, count);
    if (count > 0) {
        return count;
    }
    if (count == 0) {
        return 0;
    }

    if (errNo != NULL) {
        *errNo = errno;
    }
    switch(errno) {
        case EINTR:
        case EAGAIN:
            break;
        default:
            FATAL_LOG("read socket fd[%d] to[%p] return %ld with errno[%d][%s], this socket is disconnected", 
                    mSockfd, buf, count, errno, strerror(errno));
    };
    return -1;
}

}
