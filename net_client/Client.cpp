
#include "Client.h"

namespace dyc
{

Connection* Client::connect(const InetAddress& addr) {
    //        mEpoller = boost::shared_ptr<Epoller>(NEW Epoller());
    mEpoller = NEW Epoller();
    mEpoller->createEpoll();

    //        mLoop = boost::shared_ptr<EventLoop>(NEW EventLoop(mEpoller));
    mLoop = NEW EventLoop(mEpoller);
    mSock = NEW Socket(false);

    Connection* conn = NEW Connection(mSock, mLoop);
    mEpoller->addRW(conn);

    mSock->connect(addr);
    return conn;
}

void Client::start () {
    pthread_t ntid;
    pthread_create(&ntid, NULL, thr_fn, mLoop);
}

void* Client::thr_fn(void* data) {
    EventLoop* p = (EventLoop*)(data);
    p->loop();
}

int Client::send(const char* data, int64_t size) {
    DelayFunctor func = boost::bind(&Connection::addBuffer, mConnection, data, size);
    mLoop->queueInLoop(func);
    return 0;
}

int Client::send(const std::string& str) {
    return send(str.c_str(), str.size());
}

}
