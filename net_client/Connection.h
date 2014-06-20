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
#include "common.h"

namespace dyc {

class EventLoop;

class Connection {
private:
    static int defaultWriteCallback() {}
    static int defaultConnCallback() {}
    static int defaultReadCallback(Buffer&) {}

public:
    static const int CONN_REMOVE = -1;
    static const int CONN_CONTINUE = 0;
    static const int CONN_UPDATE = 1;
public:
//    typedef boost::shared_ptr<Socket> SocketPtr;
    typedef Socket* SocketPtr;
    typedef Buffer* BufferPtr;

    typedef boost::function< int (Buffer&) > ReadCallbackFunc;
    typedef boost::function< int () > ConnCallbackFunc;
    typedef boost::function< int () > WriteCallbackFunc;

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

    int readSocket();
    int writeSocket();
    int _writeSocket(BufferPtr buffer);

    void addBuffer(const char* data, int64_t size);
    void takeBuffer();

private:
    InetAddress localAddr;
    InetAddress peerAddr;
    SocketPtr mSocket;

//    boost::shared_ptr<EventLoop> _loop;
    EventLoop* _loop;

    WriteCallbackFunc mWriteCallback;
    ReadCallbackFunc mReadCallback;
    ConnCallbackFunc mConnCallback;

    bool mConnected;
    int mEvents;
    Buffer mReadBuffer;
    std::list<BufferPtr> mSendBuffers;
    size_t mSendInqueue;
    size_t mSendBySocket;
    
    SpinLock mLock;
    BufferPtr getSendBuffer();
};

}

#endif
