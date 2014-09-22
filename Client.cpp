
#include "Client.h"

namespace dyc
{

int Client::connect(const InetAddress& addr) {
    // mEpoller = boost::shared_ptr<Epoller>(NEW Epoller());
    mEpoller = NEW Epoller();
    mLoop = NEW EventLoop(mEpoller);
    mSock = NEW Socket(false);
    if (mEpoller == NULL || mLoop == NULL || mSock == NULL) {
        FATAL_LOG("new obj failed");
        return false;
    }

    // mLoop = boost::shared_ptr<EventLoop>(NEW EventLoop(mEpoller));
    // mSock = NEW Socket(true);

    mEpoller->createEpoll();
    mConnection = NEW Connection(mSock, mLoop);
    mEpoller->addRW(mConnection);
    return mSock->connect(addr);
}

void Client::start () {
    pthread_t ntid;
    pthread_create(&ntid, NULL, thr_fn, mLoop);
}

void* Client::thr_fn(void* data) {
    EventLoop* p = (EventLoop*)(data);
    p->loop();
}

int64_t Client::send(const char* data, int64_t size) {
    Buffer* buffer = new Buffer(data, (size_t)size);
    buffer->setMesgId(mMesgId);
    DelayFunctor func = boost::bind(&Connection::addBufferToSendQueue, mConnection, buffer);
    ++mMesgId;
    mLoop->queueInLoop(func);
    return mMesgId;
}

int64_t Client::send(const std::string& str) {
    return send(str.c_str(), str.size());
}

}
