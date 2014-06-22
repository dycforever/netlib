#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <list>
#include <sys/epoll.h>
#include <semaphore.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "InetAddress.h"
#include "Mutex.h"
#include "SpinLock.h"
#include "Socket.h"
#include "Buffer.h"
#include "Channel.h"
#include "common.h"

namespace dyc {

class EventLoop;

class Connection : public Channel {
private:
    static int defaultWriteCallback(Buffer&) {}
    static int defaultConnCallback() {}
    static int defaultReadCallback(Buffer&, Buffer&) {}

public:
//    typedef boost::shared_ptr<Socket> SocketPtr;
    typedef Socket* SocketPtr;
//    typedef Buffer* BufferPtr;

    typedef boost::function< int (Buffer&, Buffer&) > ReadCallbackFunc;
    typedef boost::function< int () > ConnCallbackFunc;
    typedef boost::function< int (Buffer&) > WriteCallbackFunc;

    explicit Connection(SocketPtr, EventLoop*);

    int getEvents() { return mEvents;}
    void setEvents(int events) { mEvents = events;}
    int fd() { return mSocket->fd();}

    void setReadCallback(ReadCallbackFunc cb) { mReadCallback = cb;}
    void setConnCallback(ConnCallbackFunc cb) { mConnCallback = cb;}
    void setWriteCallback(WriteCallbackFunc cb) { mWriteCallback = cb;}

    int handle(const epoll_event& event);

    bool isConnected() {return mConnected;}
    bool setConnected(bool stat) {return mConnected=stat;}

    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();

    long readSocket();
    int writeSocket();
    long _writeSocket(Buffer& buffer);

    void addBufferToSendQueue(const char* data, size_t size);
    void addBufferToSendQueue(Buffer);
    int64_t takeOffBuffer();

private:
    InetAddress localAddr;
    InetAddress peerAddr;
    bool mConnected;
    SocketPtr mSocket;
//    boost::shared_ptr<EventLoop> mLoop;
    EventLoop* mLoop;

    WriteCallbackFunc mWriteCallback;
    ReadCallbackFunc mReadCallback;
    ConnCallbackFunc mConnCallback;

    int mEvents;
    Buffer mRecvBuffer;
    Buffer mOutputBuffer;
    std::list<Buffer> mSendBuffers;
    size_t mSendInqueue;
    size_t mSendBySocket;
    
    SpinLock mLock;
    Buffer& getSendBuffer();
};

}

#endif
