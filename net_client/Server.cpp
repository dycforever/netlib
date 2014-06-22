
#include <boost/bind.hpp> 
#include "Server.h"

namespace dyc {

Server::Server(const InetAddress& listenAddr):
    mListenAddr(listenAddr) {
        mEpoller = NEW Epoller();
    }
Server::~Server() {}

 
Connection* Server::newConnection(Socket* socket) {
    Connection* conn = NEW Connection(socket, mLoop);
    conn->setReadCallback(mReadCallback);
    conn->setWriteCallback(mWriteCallback);
    mConnections.insert(conn);
    mEpoller->addRead(conn);
    return conn;
}
// 
// Connection* Server::connect(const InetAddress& addr) {
//     int sockfd = createBlockingSocket();
//     Socket* socket = NEW Socket(sockfd);
// 
//     int ret = socket->connect(addr);
//     assert(ret != -1);
//     return newConnection(socket);
// }
// 
// void Server::stop() {
//     mLoop->quit();
// }
// 
int Server::start() {
    mListenSocket = NEW Socket(false);
    assert(mListenSocket != NULL);

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
