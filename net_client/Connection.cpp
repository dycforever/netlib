
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "netutils/netutils.h"

namespace dyc {

Connection::Connection(SocketPtr socket, EventLoop* loop): 
    mConnected(false), mSocket(socket), mLoop(loop), 
    mRecvBuffer(NULL), mOutputBuffer(NULL), mSendInqueue(0), mSendBySocket(0) { 
        mRecvBuffer->makeSpace(1024*1024);
        mWriteCallback = boost::bind(&defaultWriteCallback, mOutputBuffer);
        mReadCallback = boost::bind(&defaultReadCallback, _1, _2);
        mConnCallback = boost::bind(&defaultConnCallback);
    }

void Connection::addBufferToSendQueue(Buffer* buffer) {
    LockGuard<SpinLock> g(mLock);
    mSendInqueue += buffer->readableSize();
    DEBUG_LOG("add buffer in send queue");
    mSendBuffers.push_back(buffer);
    enableWrite();
}

void Connection::addBufferToSendQueue(const char* data, size_t size) {
    LockGuard<SpinLock> g(mLock);
    mSendInqueue += size;
    DEBUG_LOG("add buffer in send queue");
    Buffer* buffer = NEW Buffer(data, size, true);
    mSendBuffers.push_back(buffer);
    enableWrite();
}

int64_t Connection::takeOffBuffer() {
    LockGuard<SpinLock> g(mLock);
    DEBUG_LOG("remove buffer in send queue");
    mSendBuffers.pop_front();
}

Buffer* Connection::getSendBuffer() {
    LockGuard<SpinLock> g(mLock);
    if (mSendBuffers.size() == 0) {
        return NULL;
    }
    return mSendBuffers.front();
}


long Connection::readSocket() {
    // TODO: if buffer size == 0 ?
    long ret = mSocket->recv(mRecvBuffer->beginWrite(), mRecvBuffer->writableSize());
    DEBUG_LOG("read %ld bytes", ret);
    if (ret > 0) {
        mRecvBuffer->hasWriten(static_cast<size_t>(ret));
    }
    return ret;
}


long Connection::_writeSocket(Buffer* buffer) {
    long ret = mSocket->send(buffer->beginRead(), buffer->readableSize());
    DEBUG_LOG("write %ld bytes", ret);
    size_t len = static_cast<size_t>(ret);
    if (ret > 0) {
        mSendBySocket += len;
        buffer->get(len);
    }
    return ret;
}

int Connection::writeSocket() {
    long ret = CONN_CONTINUE;
    Buffer* buffer = getSendBuffer();
    DEBUG_LOG("get buffer of size: %lu", buffer->readableSize());
    while (true) {
        if (buffer == NULL || buffer->readableSize()==0) {
            ret = CONN_UPDATE;
            disableWrite();
            break;
        }
        // TODO use writev
        ret = _writeSocket(buffer);
        if (ret < 0) {
            if (errno != EAGAIN) {
                WARN_LOG("send data failed");
                mConnected = false;
                ret = CONN_REMOVE;
            } else {
                DEBUG_LOG("send data delay");
                ret = CONN_UPDATE; 
            }
            break;
        }
        if (!buffer->readableSize()) {
            int64_t mId = takeOffBuffer();
            mWriteCallback(buffer);
            // TODO delete buffer ?
        }
        buffer = getSendBuffer();
        DEBUG_LOG("get buffer of size: %lu", buffer->readableSize());
    }
    return ret;
}

int Connection::handle(const epoll_event& event) {
    int ret = CONN_CONTINUE;
    long readCount = 0;
    if (event.events & EPOLLIN) {
        readCount = readSocket();
        DEBUG_LOG("handle in event, read %ld bytes", readCount);
        if (readCount <= 0) {
            ret = CONN_REMOVE;
            mConnected = false;
            mRecvBuffer->setFinish();
        } else {
            ret = CONN_CONTINUE;
        }
        mOutputBuffer->reset();
        mReadCallback(mRecvBuffer, mOutputBuffer);
        if (mOutputBuffer->readableSize() > 0) {
            mSendBuffers.push_back(mOutputBuffer);
        }
    } 

    if (event.events & EPOLLOUT || mSendBuffers.size() > 0) {
        if (!mConnected) {
            DEBUG_LOG("handle conn event");
            if (mSocket->checkConnected()) {
                mConnected = true;
                mConnCallback();
            } else {
                mConnected = false;
                ret = CONN_REMOVE;
            }
            return ret;
        } 
        DEBUG_LOG("handle write event");
        ret = writeSocket();
    } else {
        WARN_LOG("unknow event");
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
