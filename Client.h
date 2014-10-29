
#include <boost/bind.hpp>

#include "common.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Buffer.h"
#include "Connection.h"

namespace dyc
{

class Client {
    typedef Socket* SocketPtr;
    typedef boost::function<void()> DelayFunctor;
    typedef boost::function< int (Buffer*, Buffer*) > ReadCallbackFunc;
    typedef boost::function< int (bool) > ConnCallbackFunc;
    typedef boost::function< int (Buffer*) > WriteCallbackFunc;

public:
    Client():mMesgId(0) { }
    ~Client();

    int connect(const InetAddress& addr);
    static void* thr_fn(void* data);
    void start ();

    void setReadCallback(ReadCallbackFunc cb) { mConnection->setReadCallback(cb);}
    void setConnCallback(ConnCallbackFunc cb) { mConnection->setConnCallback(cb);}
    void setWriteCallback(WriteCallbackFunc cb) { mConnection->setWriteCallback(cb);}

    int64_t send(const char* data, int64_t size);
    int64_t send(const std::string& str);

private:
    pthread_t mTid;
    SocketPtr mSock;
//    boost::shared_ptr<EventLoop> mLoop;
//    boost::shared_ptr<Epoller> mEpoller;
    EventLoop* mLoop;
    Epoller* mEpoller;
    Connection* mConnection;
    int64_t mMesgId;
};

}
