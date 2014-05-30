#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <list>
#include <sys/epoll.h>
#include <semaphore.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "InetAddress.h"
#include "Mutex.h"
#include "Socket.h"

namespace dyc {

class EventLoop;

class Connection {
public:
//    typedef boost::shared_ptr<Socket> SocketPtr;
    typedef Socket* SocketPtr;

    typedef boost::function< int (SocketPtr) > ReadCallbackFunc;
    typedef boost::function< int (SocketPtr) > ConnCallbackFunc;
    typedef boost::function<int (SocketPtr) > WriteCallbackFunc;

    explicit Connection(SocketPtr, boost::shared_ptr<EventLoop>);

    int send(const char* data, int64_t size);

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

private:
    int sendData(SocketPtr socket);
    void writeComplete();
    void readComplete();

    MutexLock _listMutex;

    InetAddress localAddr;
    InetAddress peerAddr;
    SocketPtr mSocket;

    boost::shared_ptr<EventLoop> _loop;

    WriteCallbackFunc mWriteCallback;
    ReadCallbackFunc mReadCallback;
    ConnCallbackFunc mConnCallback;

    ReadCallbackFunc _errorCallback;

    bool mConnected;
    int mEvents;
};

}

#endif
