#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <list>
#include <sys/epoll.h>
#include <semaphore.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "thread/ThreadLock.h"

#include "InetAddress.h"
#include "Socket.h"
#include "Buffer.h"
#include "Channel.h"
#include "common.h"

namespace dyc {

class EventLoop;

class Connection : public Channel {
private:
    enum {
        RET_WRITE_BUFFER_FULL = -2,
        RET_HAS_ERROR = -1
    };
public:
//    typedef boost::shared_ptr<Socket> SocketPtr;
    typedef Socket* SocketPtr;
//    typedef Buffer* BufferPtr;

    typedef boost::function< int (Buffer*, Buffer*) > ReadCallbackFunc;
    typedef boost::function< int (bool) > ConnCallbackFunc;
    typedef boost::function< int (Buffer*) > WriteCallbackFunc;

    explicit Connection(SocketPtr, EventLoop*);
    virtual ~Connection();

    int getEvents() { return mEvents;}
    void setEvents(int events) { mEvents = events;}
    int fd() { return mSocket->fd();}

    void setReadCallback(ReadCallbackFunc cb) { mReadCallback = cb;}
    void setConnCallback(ConnCallbackFunc cb) { mConnCallback = cb;}
    void setWriteCallback(WriteCallbackFunc cb) { mWriteCallback = cb;}

    int handle(const epoll_event& event);
    int handleRead(const epoll_event& event);
    int handleConnect(const epoll_event& event);
    int handleWrite(const epoll_event& event);

    bool isConnected() {return mConnected;}
    bool setConnected(bool stat) {return mConnected=stat;}
    void enableRead();
    void disableRead();
    void enableWrite();
    void disableWrite();

    long readSocket(int*);
    long writeSocket(Buffer* buffer);

    size_t getSendBufferCount ();
    void addBufferToSendQueue(const char* data, size_t size);
    void addBufferToSendQueue(Buffer*);
    int64_t takeOffBuffer();

private:
//    static int defaultWriteCallback(Buffer*) { std::cout << "call Connection::defaultWriteCallback" << std::endl;}
//    static int defaultConnCallback(bool) { std::cout << "call Connection::defaultConnCallback" << std::endl;}
//    static int defaultReadCallback(Buffer*, Buffer*) { std::cout << "call Connection::defaultReadCallback" << std::endl;}
    static int defaultWriteCallback(Buffer*) { return 0;}
    static int defaultConnCallback(bool) { return 0;}
    static int defaultReadCallback(Buffer*, Buffer*) { return 0;}

private:
    InetAddress localAddr;
    InetAddress peerAddr;
    bool mConnected;
    SocketPtr mSocket;
    EventLoop* mLoop;

    WriteCallbackFunc mWriteCallback;
    ReadCallbackFunc mReadCallback;
    ConnCallbackFunc mConnCallback;

    int mEvents;
    Buffer* mRecvBuffer;
    std::list<Buffer*> mSendBuffers;
    size_t mSendInqueue;
    size_t mSendBySocket;
    
    Buffer* getSendBuffer();
};

}

#endif
