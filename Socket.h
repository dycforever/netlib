
#include <sys/epoll.h>
#include <netinet/ip_icmp.h>

#include "common.h"
#include "netutils/Log.h"

#ifndef __SOCKET_H__
#define __SOCKET_H__

namespace dyc {

class InetAddress;
// Wrapper of socket file descriptor.
//
// It closes the sockfd when desctructs.
// It's thread safe, all operations are delagated to OS.
class Socket {
public:
    void createSocket() {
        mSockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (mSockfd < 0) {
            FATAL_LOG("createNonblock failed, errno:%d with %s", errno, strerror(errno));
        }
    }

    void setNonblocking() {
        int opts = fcntl(mSockfd, F_GETFL);
        if (opts < 0) {
            FATAL_LOG("Executing fcntl function(getting flags) failed.  errno:%d with %s", errno, strerror(errno));
        }

        opts = opts | O_NONBLOCK;
        if (fcntl(mSockfd, F_SETFL, opts) < 0 ) {
            FATAL_LOG("Executing fcntl function(setting flags) failed.  errno:%d with %s", errno, strerror(errno));
        }
    }


    explicit Socket(bool blocking)
        : mBlocking(blocking){ 
            createSocket();
            if (!mBlocking) {
                setNonblocking();
            }
        }
    explicit Socket(int socket): mSockfd(socket) {;}
    ~Socket();

    int bind(const InetAddress& localaddr);
    int listen();
    int accept(InetAddress& peeraddr);
    int connect(const InetAddress& localaddr);

    int fd() {return mSockfd;}
    int shutdownWrite();

    /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
    void setTcpNoDelay(bool on);
    /// Enable/disable SO_REUSEADDR
    void setReuseAddr(bool on);
    /// Enable/disable SO_KEEPALIVE
    void setKeepAlive(bool on);

    int send(const char* buf, size_t len);
    long recv(char* buf, size_t len);

    bool getPeerAddr(InetAddress& addr);
    bool checkConnected();

    int getopt(int level, int optname, void* optval, void* len);
    int setopt(int level, int optname, void* optval, socklen_t len);

    ssize_t sendmsg(const struct ::msghdr *msg, int flags);
    ssize_t recvmsg(struct ::msghdr *msg, int flags);
private:
    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    int accept(int sockfd, struct sockaddr_in* addr);

    int mSockfd;
    bool mBlocking;
};

}

#endif
