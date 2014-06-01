
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero

#include "InetAddress.h"
#include "Socket.h"  
#include "log.h"  

namespace dyc {

ssize_t Socket::sendmsg(const struct msghdr *msg, int flags) {
    return ::sendmsg(mSockfd, msg, flags);
}

ssize_t Socket::recvmsg(struct msghdr *msg, int flags) {
    return ::recvmsg(mSockfd, msg, flags);
}

Socket::~Socket()
{
    if (close(mSockfd) == 0) {
        NOTICE("close %d success", mSockfd);
    } else {
        FATAL("close %d failed: %d %s", mSockfd, errno, strerror(errno));
    }
}

int Socket::getopt(int level, int optname, void* optval, void* len) {
    int ret = ::getsockopt(mSockfd, level, optname, optval, (socklen_t*)len);
    if (ret < 0) {
        FATAL("getsockopt() failed, ret:%d socket[%d] level[%d] optname[%u] errno[%d] with %s", 
                ret, mSockfd, level, optname, errno, strerror(errno));
        return ret;
    }
    return ret;
}

int Socket::setopt(int level, int optname, void* optval, socklen_t len) {
    int ret = ::setsockopt(mSockfd, level, optname, optval, len);
    if (ret < 0) {
        FATAL("setsockopt() failed, ret:%d socket[%d] level[%d] optname[%u] errno[%d] with %s", 
                ret, mSockfd, level, optname, errno, strerror(errno));
    }
    return ret;
}

int Socket::connect(const InetAddress& peerAddr) {
   const struct sockaddr_in& sockAddr = peerAddr.getSockAddrInet();
   int ret = ::connect(mSockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
   if ( mBlocking && ret < 0) {
        FATAL("ret:%d bind socket[%d] raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
   }
   return ret;
}

int Socket::bind(const InetAddress& addr)
{
    struct sockaddr_in sockAddr = addr.getSockAddrInet();
    int ret = ::bind(mSockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
    if (ret < 0)
    {
        FATAL("ret:%d bind socket[%d] raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
        return ret;
    }
    setReuseAddr(true);
    NOTICE("bind with socket[%d] raw_ip[%s] port[%u]", 
           mSockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
    return 0;
}

int Socket::listen()
{
    int ret = ::listen(mSockfd, 100);
    if (ret < 0) {
        FATAL("listen  Die errno:%d with %s", errno, strerror(errno));
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
                FATAL("unexpected error of ::accept %d", savedErrno);
                break;
            default:
                FATAL("unknown error of ::accept %d", savedErrno);
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
        FATAL("SO_REUSEPORT failed.");
    }
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(mSockfd, SOL_SOCKET, SO_KEEPALIVE,
            &optval, static_cast<socklen_t>(sizeof optval));
}


int Socket::send(const char* buf, size_t len) {
write_again:
    DEBUG("raw socket writing: %s", buf);
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
            NOTICE("write return %d with errno[%d], this socket is disconnected", count, errno);
    };
    return count;
}

int Socket::recv(char* buf, size_t len) {
    int count = 0;
read_again:
    count = read(mSockfd, buf, len);
    if (count > 0) {
        return count;
    }
    if (count == 0)
        return 0;
    switch(errno) {
        case EINTR:
            goto read_again;
        case EAGAIN:
            break;
        default:
            NOTICE("read return %d with errno[%d], this socket is disconnected", count, errno);
    };
    return count;
}


}
