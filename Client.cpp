
#include "Client.h"

namespace dyc
{

Client::Client()
    : mMesgId(0) 
{ 
    mEpoller = NEW Epoller();
    mLoop = NEW EventLoop(mEpoller);
    mSock = NEW Socket(false);
}

Client::~Client()
{
    mLoop->quit();
    pthread_join(mTid, NULL);
    DELETE(mConnection);
    DELETE(mSock);
    DELETE(mLoop);
    DELETE(mEpoller);
}

int Client::connect(const InetAddress& addr) {
    if (mEpoller == NULL || mLoop == NULL || mSock == NULL) {
        FATAL_LOG("new obj failed");
        return false;
    }

    mEpoller->createEpoll();
    mConnection = NEW Connection(mSock, mLoop);
    mEpoller->addRW(mConnection);
    return mSock->connect(addr);
}

void Client::start() {
    pthread_create(&mTid, NULL, thr_fn, mLoop);
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
