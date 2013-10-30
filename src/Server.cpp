#include <boost/bind.hpp> 
#include "Server.h"

namespace dyc {

Server::Server(const InetAddress& listen_addr):
    _listenAddr(listen_addr) {}
Server::~Server() {}

int Server::accepter(Socket* sock) {
    InetAddress addr;
    int newfd = sock->accept(addr);
    NOTICE("accept a new conn with addr:%s", addr.toIpPort().c_str());
    if (newfd< 0) {
        FATAL("fd[%d] errno[%d]: %s", newfd, errno, strerror(errno));
        return -1;
    }
    Socket* newso = NEW Socket(newfd);
    newso->setConnected(true);
    return (newConnection(newso) != NULL) ? 0 : -1;
}

Connection* Server::newConnection(Socket* socket) {
    Connection* conn = NEW Connection(socket, _loop);
    conn->setReadCallback(_readCallback);
    conn->setWriteCallback(_writeCallback);
    conn->setMallocCallback(_mallocCallback);
    _connections.insert(conn);
    _epoller->addRead(socket);
    return conn;
}

Connection* Server::connect(const InetAddress& addr) {
    int sockfd = createBlockingSocket();
    Socket* socket = NEW Socket(sockfd);

    int ret = socket->connect(addr);
    assert(ret != -1);
    return newConnection(socket);
}

void Server::stop() {
    _loop->quit();
}

int Server::start() {

    int sockfd = createBlockingSocket();
    assert(sockfd != -1);

    _listenSocket.reset(NEW Socket(sockfd));
    int ret = _listenSocket->bind(_listenAddr);
    assert(ret != -1);

    ret = _listenSocket->listen();
    assert(ret != -1);

    _epoller.reset(NEW Epoller());
    ret = _epoller->createEpoll();
    CHECK_ERROR(-1, ret == 0, "create epoller failed");

    ConnCallbackFunc acceptCallback = boost::bind(&Server::accepter, this, _1);
    _listenSocket->setReadCallback(acceptCallback);

    ret = _epoller->addRW(_listenSocket.get());
    CHECK_ERROR(-1, ret == 0, "add read write epoller failed");

    _loop.reset(NEW EventLoop(_epoller));
    _loop->loop();
    return 0;
}

}
