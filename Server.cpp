
#include <boost/bind.hpp> 
#include "Server.h"

namespace dyc {

Server::Server(const InetAddress& listenAddr):
    mEpoller(NULL),
    mListenSocket(NULL),
    mLoop(NULL),
    mListenAddr(listenAddr) {
        mWriteCallback = boost::bind(&defaultWriteCallback, _1);
        mReadCallback = boost::bind(&defaultReadCallback, _1, _2);
    }

Server::Server(uint16_t port):
    mEpoller(NULL),
    mListenSocket(NULL),
    mLoop(NULL),
    mListenAddr("0.0.0.0", port) {
        mWriteCallback = boost::bind(&defaultWriteCallback, _1);
        mReadCallback = boost::bind(&defaultReadCallback, _1, _2);
    }

Server::~Server() {
    DELETE(mLoop);
    DELETE(mAccepter);
    DELETE(mListenSocket);
    DELETE(mEpoller);
}

Connection* Server::newConnection(Socket* socket) {
    Connection* conn = NEW Connection(socket, mLoop);
    conn->setReadCallback(mReadCallback);
    conn->setWriteCallback(mWriteCallback);
    mConnections.insert(conn);
    mEpoller->addRead(conn);
    return conn;
}
 
void Server::stop() {
    mLoop->quit();
}

int Server::start() {
    mEpoller = NEW Epoller();
    mListenSocket = NEW Socket(false);
    assert(mListenSocket != NULL && mEpoller != NULL);

    int ret = mListenSocket->bind(mListenAddr);
    assert(ret != -1);
    ret = mListenSocket->listen();
    assert(ret != -1);

    ret = mEpoller->createEpoll();
    assert(ret != -1);

    mAccepter = NEW Accepter(mListenSocket);
    boost::function<Connection* (Socket*)> func = 
        boost::bind(&Server::newConnection, this, _1);
    mAccepter->setNewConnCallback(func);
    ret = mEpoller->addRW(mAccepter);
    assert(ret != -1);
//    CHECK_ERROR(-1, ret == 0, "add read write epoller failed");

//    mLoop.reset(NEW EventLoop(mEpoller));
    mLoop = NEW EventLoop(mEpoller);
    mLoop->loop();
    return 0;
}

}
