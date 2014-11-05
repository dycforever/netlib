
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "netutils/netutils.h"

namespace dyc {

Connection::Connection(SocketPtr socket, EventLoop* loop): 
    mConnected(false), mSocket(socket), mLoop(loop), 
    mRecvBuffer(NULL), mOutputBuffer(NULL), mSendInqueue(0), mSendBySocket(0) { 
        mRecvBuffer = new Buffer();
        mOutputBuffer = new Buffer();
        mWriteCallback = boost::bind(&defaultWriteCallback, mOutputBuffer);
        mReadCallback = boost::bind(&defaultReadCallback, _1, _2);
        mConnCallback = boost::bind(&defaultConnCallback, _1);
}

Connection::~Connection() {
    DELETE(mRecvBuffer);
    DELETE(mOutputBuffer);
}

void Connection::addBufferToSendQueue(Buffer* buffer) {
    mSendInqueue += buffer->readableSize();
    DEBUG_LOG("add buffer in send queue");
    mSendBuffers.push_back(buffer);
    enableWrite();
}

void Connection::addBufferToSendQueue(const char* data, size_t size) {
    mSendInqueue += size;
    DEBUG_LOG("add data in send queue");
    Buffer* buffer = NEW Buffer(data, size);
    mSendBuffers.push_back(buffer);
    enableWrite();
}

int64_t Connection::takeOffBuffer() {
    DEBUG_LOG("remove buffer in send queue");
    mSendBuffers.pop_front();
}

Buffer* Connection::getSendBuffer() {
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


long Connection::writeSocket(Buffer* buffer) {
    long ret = mSocket->send(buffer->beginRead(), buffer->readableSize());
    DEBUG_LOG("write %ld bytes", ret);
    size_t len = static_cast<size_t>(ret);
    if (ret > 0) {
        mSendBySocket += len;
        buffer->get(len);
    }
    return ret;
}

int Connection::handleWrite(const epoll_event& event) {
    long ret = CONN_CONTINUE;
    Buffer* buffer = getSendBuffer();
    while (true) {
        if (buffer == NULL || buffer->readableSize()==0) {
            disableWrite();
            break;
        }
        DEBUG_LOG("get buffer of size: %lu", buffer->readableSize());
        // TODO use writev
        ret = writeSocket(buffer);
        if (ret < 0) {
            if (errno != EAGAIN) {
                WARN_LOG("send data failed");
                mConnected = false;
                ret = CONN_REMOVE;
            } else {
                DEBUG_LOG("send data later");
                ret = CONN_CONTINUE;
            }
            break;
        }
        if (!buffer->readableSize()) {
            int64_t mId = takeOffBuffer();
            mWriteCallback(buffer);
            DELETE(buffer);
        }
        buffer = getSendBuffer();
    }
    return (int)ret;
}

int Connection::handleRead(const epoll_event& event) {
    int ret = CONN_CONTINUE;
    long readCount = 0;
    long readret = 0;
    do {
        readret = readSocket();
        if ((readret == 0) || 
                (readret < 0 && errno != EAGAIN)) {
            mConnected = false;
            mRecvBuffer->setFinish();
            // for debug
            ret = CONN_REMOVE;
        }
        readCount += readret;
    } while(readret > 0);

    mOutputBuffer->reset();
    mReadCallback(mRecvBuffer, mOutputBuffer);
    if (mOutputBuffer->readableSize() > 0) {
        addBufferToSendQueue(mOutputBuffer);
    }
    return ret;
}

int Connection::handleConnect(const epoll_event& event) {
    int ret = CONN_CONTINUE;
    DEBUG_LOG("handle conn event");
    if (mSocket->checkConnected()) {
        mConnected = true;
        mConnCallback(true);
        ret = CONN_CONTINUE;
    } else {
        mConnected = false;
        mConnCallback(false);
        ret = CONN_REMOVE;
    }
    return ret;
}

size_t Connection::getSendBufferCount() {
    return mSendBuffers.size();
}


int Connection::handle(const epoll_event& event) {
    int ret = CONN_CONTINUE;

    if (event.events & EPOLLOUT || getSendBufferCount() > 0) {
        if (!mConnected) {
            ret = handleConnect(event);
            if (ret != CONN_CONTINUE) {
                return ret;
            }
        } 
        DEBUG_LOG("handle write event");
        ret = handleWrite(event);
        if (ret != CONN_CONTINUE) {
            return ret;
        }
    }

    if (event.events & EPOLLIN) {
        DEBUG_LOG("handle read event");
        ret = handleRead(event);
        if (ret != CONN_CONTINUE) {
            return ret;
        }
    } 

    if(event.events & ~(EPOLLIN | EPOLLOUT)) {
        WARN_LOG("unknow event");
    }

    if (getSendBufferCount() > 0) {
        return handleWrite(event);
    }
    return ret;
}

void Connection::enableRead() { 
    mEvents |= EPOLLIN; 
    mLoop->updateChannel(this);
}

void Connection::disableRead() { 
    mEvents &= (~EPOLLIN); 
    mLoop->updateChannel(this);
}

void Connection::enableWrite() { 
    mEvents |= EPOLLOUT; 
    mLoop->updateChannel(this);
}

void Connection::disableWrite() { 
    mEvents &= (~EPOLLOUT); 
    mLoop->updateChannel(this);
}


}
