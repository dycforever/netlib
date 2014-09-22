
#ifndef ACCEPTER_H
#define ACCEPTER_H

#include <Channel.h>
#include <InetAddress.h>

namespace dyc {

class Accepter : public Channel {
public:
    typedef boost::function<Connection* (Socket*)> NewConnCallback;
    typedef Socket* SocketPtr;
    Accepter(SocketPtr sock):mSocket(sock) {}
    virtual ~Accepter() {}

    int accept() {
        InetAddress addr;
        int newfd = mSocket->accept(addr);
        NOTICE_LOG("accept a new conn with addr:%s", addr.toIpPort().c_str());
        if (newfd< 0) {
            FATAL_LOG("fd[%d] errno[%d]: %s", newfd, errno, strerror(errno));
            return -1;
        }
        Socket* newso = NEW Socket(newfd);
        return (mNewConnCallback(newso) != NULL) ? 0 : -1;
    }

    void setNewConnCallback(NewConnCallback cb) {
        mNewConnCallback = cb;
    }

    int fd() {
        return mSocket->fd();
    }
    int getEvents() { /* trival*/ return EPOLLIN; }
    void setEvents(int) { /* trival*/ return ; }

    int handle(const epoll_event& event) {
        int ret = CONN_CONTINUE;
        long readCount = 0;
        if (event.events & EPOLLIN)  {
            ret = accept();
        } else {
            WARN_LOG("strange! listen socket found %d event", event.events);
        }
        return ret;
    }

private:
    SocketPtr mSocket;
    NewConnCallback mNewConnCallback;
};

}

#endif
