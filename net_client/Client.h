
#include "common.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <boost/bind.hpp>

namespace dyc
{

class Client {
    typedef Socket* SocketPtr;
    typedef boost::function<void()> DelayFunctor;
    typedef boost::function< int (Buffer&) > ReadCallbackFunc;
    typedef boost::function< int () > ConnCallbackFunc;
    typedef boost::function< int () > WriteCallbackFunc;

public:
    Client() { }

    Connection* connect(const InetAddress& addr);
    static void* thr_fn(void* data);
    void start ();

int send(const char* data, int64_t size);

int send(const std::string& str);

private:
    SocketPtr mSock;
//    boost::shared_ptr<EventLoop> mLoop;
//    boost::shared_ptr<Epoller> mEpoller;
    EventLoop* mLoop;
    Epoller* mEpoller;
    Connection* mConnection;
};

}
