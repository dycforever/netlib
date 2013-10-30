
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero

#include "InetAddress.h"
#include "Socket.h"  

namespace dyc {

Socket::~Socket()
{
    if (close(_sockfd) == 0) {
        NOTICE("close %d success", _sockfd);
    } else {
        FATAL("close %d failed: %d %s", _sockfd, errno, strerror(errno));
    }
}

int Socket::connect(const InetAddress& peerAddr) {
   const struct sockaddr_in& sockAddr = peerAddr.getSockAddrInet();
   int ret = ::connect(_sockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
   if (ret < 0) {
        FATAL("ret:%d bind socket[%d] raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, _sockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
   }
   _connected = true;
   return ret;
}

int Socket::bind(const InetAddress& addr)
{

    struct sockaddr_in sockAddr = addr.getSockAddrInet();
    int ret = ::bind(_sockfd, sockaddr_cast(&sockAddr), static_cast<socklen_t>(sizeof sockAddr));
    if (ret < 0)
    {
        FATAL("ret:%d bind socket[%d] raw_ip[%s] port[%u] Die errno[%d] with %s", 
                ret, _sockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port), errno, strerror(errno));
        return ret;
    }
    setReuseAddr(true);
    NOTICE("bind with socket[%d] raw_ip[%s] port[%u]", 
           _sockfd, inet_ntoa(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
    return 0;
}

int Socket::listen()
{
    int ret = ::listen(_sockfd, 100);
    if (ret < 0)
    {
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

    if (connfd < 0)
    {
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
    int connfd = accept(_sockfd, &addr);
    if (connfd >= 0) {
        peeraddr.setSockAddrInet(addr);
    }
    return connfd;
}

int Socket::shutdownWrite()
{
    return shutdown(_sockfd, SHUT_WR);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY,
            &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR,
            &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0)
    {
        FATAL("SO_REUSEPORT failed.");
    }
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE,
            &optval, static_cast<socklen_t>(sizeof optval));
}

// return 0 means the socket can be removed
int Socket::handle(const epoll_event& event) {
    int ret = 0;
    if (event.events & EPOLLIN) {
        ret = handleRead();
    } else if (event.events & EPOLLOUT) {
        ret = handleWrite();
    } else {
        ret = -1;
        NOTICE("unknow event");
    }
    if (ret < 0) {
        handleError();
    }
    return ret;
}

void Socket::enableRead() {
    _events |= EPOLLIN;
}

void Socket::disableRead() {
    _events &= (~EPOLLIN);
}

void Socket::enableWrite() {
    _events |= EPOLLOUT;
}

void Socket::disableWrite() {
    _events &= (~EPOLLOUT);
}


int Socket::send(const char* buf, size_t len) {
write_again:
    int count = write(_sockfd, buf, len);
    if (count > 0) {
        return count;
    }
    switch(errno) {
        case EINTR:
        case EAGAIN:
            goto write_again;
        default:
            WARNING("write return %d with errno[%d], this socket is disconnected", count, errno);
            _connected = false;
    };
    return count;
}

int Socket::recv(char* buf, size_t len) {
    int count = 0;
read_again:
    count = read(_sockfd, buf, len);
    if (count > 0) {
        return count;
    }
    if (count == 0)
        return 0;
    switch(errno) {
        case EINTR:
        case EAGAIN:
            goto read_again;
        default:
            WARNING("read return %d with errno[%d], this socket is disconnected", count, errno);
            _connected = false;
    };
    return count;
}

int Socket::handleError() {
    DEBUG("call error callback");
    return 0;
//    return _errorCallback();
}

int Socket::handleRead() {
    return _readCallback(this);
}

int Socket::handleWrite() {
    return _writeCallback(this);
}

}
