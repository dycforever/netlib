
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "util.h"

namespace dyc {

Connection::Connection(SocketPtr socket, EventLoop* loop): 
    mConnected(false), mSocket(socket), _loop(loop), 
    mReadBuffer(NULL, 0) { 
        mReadBuffer.makeSpace(1024*1024);
        mWriteCallback = boost::bind(&defaultWriteCallback);
        mReadCallback = boost::bind(&defaultReadCallback, _1);
        mConnCallback = boost::bind(&defaultConnCallback);
    }

void Connection::addBuffer(const char* data, int64_t size) {
    LockGuard<SpinLock> g(mLock);
    DEBUG("add buffer in send queue");
    mSendBuffers.push_back(NEW Buffer(data, size, true));
}

void Connection::removeBuffer() {
    LockGuard<SpinLock> g(mLock);
    DEBUG("remove buffer in send queue");
    DELETE(mSendBuffers.front());
    mSendBuffers.pop_front();
}

int Connection::send(const char* data, int64_t size) {
    addBuffer(data, size);
    enableWrite();
    return 0;
}

int Connection::send(const std::string& str) {
    return send(str.c_str(), str.size());
}


Connection::BufferPtr Connection::getSendBuffer() {
    LockGuard<SpinLock> g(mLock);
    if (mSendBuffers.size() == 0) {
        return NULL;
    }
    return mSendBuffers.front();
}


int Connection::readSocket() {
    int ret = mSocket->recv(mReadBuffer.beginWrite(), mReadBuffer.writableSize());
    DEBUG("read %d bytes", ret);
    mReadBuffer.hasWriten(ret);
    return ret;
}


int Connection::_writeSocket(BufferPtr buffer) {
    int ret = mSocket->send(buffer->beginRead(), buffer->readableSize());
    DEBUG("write %d bytes", ret);
    if (ret > 0)
        buffer->get(ret);
    return ret;
}

int Connection::writeSocket() {
    int ret = CONN_CONTINUE;
    BufferPtr buffer = getSendBuffer();
    DEBUG("get buffer: %p of size: %lu", buffer, buffer?buffer->readableSize():0);
    while (true) {
        if (!buffer || buffer->readableSize()==0) {
            ret = CONN_UPDATE;
            disableWrite();
            break;
        }
        // TODO use writev
        ret = _writeSocket(buffer);
        if (ret < 0) {
            if (errno != EAGAIN) {
                mConnected = false;
                ret = CONN_REMOVE;
            } else {
                ret = CONN_UPDATE; 
            }
            break;
        }
        if (!buffer->readableSize()) {
            removeBuffer();
            mWriteCallback();
        }
        buffer = getSendBuffer();
        DEBUG("get buffer: %p of size: %lu", buffer, buffer?buffer->readableSize():0);
    }
    return ret;
}

// return -1 means the socket can be removed
// return 1 means the socket need be updated
int Connection::handle(const epoll_event& event) {
    int ret = CONN_CONTINUE;
    int readCount = 0;
    if (event.events & EPOLLIN) {
        readCount = readSocket();
        DEBUG("handle in event, read %d bytes", readCount);
        if (readCount <= 0) {
            ret = CONN_REMOVE;
            mConnected = false;
        } else {
            ret = CONN_CONTINUE;
            mReadCallback(mReadBuffer);
        }
    } else if (event.events & EPOLLOUT) {
        if (!mConnected) {
            DEBUG("handle conn event");

            if (mSocket->checkConnected()) {
                mConnected = true;
                mConnCallback();
            } else {
                mConnected = false;
                ret = CONN_REMOVE;
            }
        } else {
            DEBUG("handle write event");
            ret = writeSocket();
        }
    } else {
        INFO("unknow event");
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
