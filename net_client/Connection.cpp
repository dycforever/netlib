
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "util.h"

namespace dyc {

Connection::Connection(SocketPtr socket, boost::shared_ptr<EventLoop> loop): mSocket(socket), _loop(loop) { }


int Connection::send(const char* data, int64_t size) {
    return 0;
}

void Connection::writeComplete() {
}

// return 0 means the socket can be removed
int Connection::handle(const epoll_event& event) {
    int ret = 0;
    if (event.events & EPOLLIN) {
        mReadCallback(mSocket);
    } else if (event.events & EPOLLOUT) {
        mWriteCallback(mSocket);
    } else {
        ret = -1;
        NOTICE("unknow event");
    }
    if (ret < 0) {
        // handleError();
    }
    return ret;
}

void Connection::enableRead() {
    mEvents |= EPOLLIN;
}

void Connection::disableRead() {
    mEvents &= (~EPOLLIN);
}

void Connection::enableWrite() {
    mEvents |= EPOLLOUT;
}

void Connection::disableWrite() {
    mEvents &= (~EPOLLOUT);
}

}
