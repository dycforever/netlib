#include <sys/epoll.h>
#include <boost/function.hpp>
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
    typedef boost::function<int(Socket*)> CallbackFunc;

    explicit Socket(int sockfd)
        : _sockfd(sockfd), _connected(false){ }
    ~Socket();

    int bind(const InetAddress& localaddr);
    int listen();
    int accept(InetAddress& peeraddr);
    int connect(const InetAddress& localaddr);
    int getEvents() { return _events;}
    void setEvents(int events) { _events = events;}

    int fd() {return _sockfd;}
    int shutdownWrite();

    /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
    void setTcpNoDelay(bool on);
    /// Enable/disable SO_REUSEADDR
    void setReuseAddr(bool on);
    /// Enable/disable SO_KEEPALIVE
    void setKeepAlive(bool on);


    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();

    int handle(const epoll_event& event);
    void setWriteCallback(CallbackFunc cb) {_writeCallback = cb;}
    void setReadCallback(CallbackFunc cb) {_readCallback = cb;}
    void setErrorCallback(CallbackFunc cb) {_errorCallback = cb;}

    int send(const char* buf, size_t len);
    int recv(char* buf, size_t len);

    bool isConnected() {return _connected;}
    bool setConnected(bool stat) {return _connected=stat;}
private:
    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    int accept(int sockfd, struct sockaddr_in* addr);

    int _sockfd;
    CallbackFunc _readCallback;
    CallbackFunc _writeCallback;
    CallbackFunc _errorCallback;

    int handleRead();
    int handleWrite();
    int handleError();

    int _events;
    int _revents;

    bool _connected;
};

}

#endif
