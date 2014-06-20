
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "util.h"

namespace dyc {

Connection::Connection(SocketPtr socket, EventLoop* loop): 
    mConnected(false), mSocket(socket), _loop(loop), 
    mReadBuffer(NULL, 0), mSendInqueue(0), mSendBySocket(0) { 
        mReadBuffer.makeSpace(1024*1024);
        mWriteCallback = boost::bind(&defaultWriteCallback);
        mReadCallback = boost::bind(&defaultReadCallback, _1);
        mConnCallback = boost::bind(&defaultConnCallback);
    }

void Connection::addBuffer(const char* data, int64_t size) {
    LockGuard<SpinLock> g(mLock);
    mSendInqueue += size;
    DEBUG("add buffer in send queue");
    mSendBuffers.push_back(NEW Buffer(data, size, true));
    enableWrite();
}

void Connection::takeBuffer() {
    LockGuard<SpinLock> g(mLock);
    DEBUG("remove buffer in send queue");
    DELETE(mSendBuffers.front());
    mSendBuffers.pop_front();
}



Connection::BufferPtr Connection::getSendBuffer() {
    LockGuard<SpinLock> g(mLock);
    if (mSendBuffers.size() == 0 ||
            mSendBuffers.front()->readableSize() == 0) {
//        disableWrite();
        return NULL;
    }
    return mSendBuffers.front();
}


int Connection::readSocket() {
    // TODO: if buffer size == 0 ?
    int ret = mSocket->recv(mReadBuffer.beginWrite(), mReadBuffer.writableSize());
    DEBUG("read %d bytes", ret);
    mReadBuffer.hasWriten(ret);
    return ret;
}


int Connection::_writeSocket(BufferPtr buffer) {
    int ret = mSocket->send(buffer->beginRead(), buffer->readableSize());
    DEBUG("write %d bytes", ret);
    if (ret > 0) {
        mSendBySocket += ret;
        buffer->get(ret);
    }
    return ret;
}

int Connection::writeSocket() {
    int ret = CONN_CONTINUE;
    BufferPtr buffer = getSendBuffer();
    DEBUG("get buffer: %p of size: %lu", buffer, buffer?buffer->readableSize():0);
    while (true) {
        if (!buffer || buffer->readableSize()==0) {
            ret = CONN_UPDATE;
            break;
        }
        // TODO use writev
        ret = _writeSocket(buffer);
        if (ret < 0) {
            if (errno != EAGAIN) {
                WARN("send data failed");
                mConnected = false;
                ret = CONN_REMOVE;
            } else {
                DEBUG("send data delay");
                ret = CONN_UPDATE; 
            }
            break;
        }
        if (!buffer->readableSize()) {
            takeBuffer();
            // TODO: return msg id
            mWriteCallback();
        }
        buffer = getSendBuffer();
        DEBUG("get buffer: %p of size: %lu", buffer, buffer?buffer->readableSize():0);
    }
    return ret;
}

int Connection::handle(const epoll_event& event) {
    int ret = CONN_CONTINUE;
    int readCount = 0;
    if (event.events & EPOLLIN) {
        readCount = readSocket();
        DEBUG("handle in event, read %d bytes", readCount);
        if (readCount <= 0) {
            ret = CONN_REMOVE;
            mConnected = false;
//            if (readCount == 0) {
                mReadBuffer.setFinish();
//            }
        } else {
            ret = CONN_CONTINUE;
        }
        mReadCallback(mReadBuffer);
    } 

    if (event.events & EPOLLOUT) {
        if (!mConnected) {
            DEBUG("handle conn event");
            if (mSocket->checkConnected()) {
                mConnected = true;
                mConnCallback();
            } else {
                mConnected = false;
                ret = CONN_REMOVE;
            }
            return ret;
        } 
        DEBUG("handle write event");
        ret = writeSocket();
    } else {
        WARN("unknow event");
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
