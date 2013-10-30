#include <boost/bind.hpp>
#include "Epoller.h"

namespace dyc {

Epoller::Epoller() {
    _timeout = -1;
}


void Epoller::setTimeout(int t) {
    _timeout = t;
}

int Epoller::addRead(SocketPtr socket) {
    lock();
    int ret = _addEvent(socket, EPOLLIN);
    unlock();
    return ret;
}

int Epoller::addWrite(SocketPtr socket) {
    lock();
    int ret = _addEvent(socket, EPOLLOUT);
    unlock();
    return ret;
}

int Epoller::addRW(SocketPtr socket) {
    int ret = 0;
    ret = _addEvent(socket, EPOLLIN|EPOLLOUT);
    return ret;
}

int Epoller::removeEvent(SocketPtr socket) {
    lock();
    int ret = _removeEvent(socket);
    unlock();
    return ret;
}

void Epoller::lock() {
    pthread_mutex_lock(&_mutex);
}

void Epoller::unlock() {
    pthread_mutex_unlock(&_mutex);
}

int Epoller::createEpoll() {
    pthread_mutex_init(&_mutex, NULL);
    int sock = epoll_create(EPOLL_MAX_LISTEN_NUMBER);
    if (sock < 0) {
        return sock;
    }
    NOTICE("create epoll socket[%d]", sock);
    _epoll_socket = sock;

    return 0;
}

int Epoller::_removeEvent(SocketPtr socket) {
    int sockfd = socket->fd();
    int epsfd = _epoll_socket;
    NOTICE("del port in epoll %d", epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_DEL, sockfd, NULL);
    if( ret < 0 ){
        FATAL("add socket:%d into epoll fd:%d failed", sockfd, epsfd);
        return -1;
    }
    return 0;
}

int Epoller::_addEvent(SocketPtr socket, uint32_t op_types) {
    int sockfd = socket->fd();
    socket->setEvents(op_types);

    struct epoll_event event;
    event.data.ptr = (void*)socket;
    event.events = op_types;

    int epsfd = _epoll_socket;
    const char* ev = op_types==EPOLLIN ? "epoll_in" : "epoll_out";
    NOTICE("add [%s] event in epoll socket[%d]", ev, epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_ADD, sockfd, &event);
    if( ret < 0 ){
        FATAL("add socket:%d into epoll fd:%d failed errno:%d %s", sockfd, epsfd, errno, strerror(errno));
        return -1;
    }
    return 0;
}

int Epoller::poll(Event* list) {
    NOTICE("epoll wait on socket[%d] size:%d", _epoll_socket, 10);
    int ret = epoll_wait(_epoll_socket, list, EPOLL_MAX_LISTEN_NUMBER, _timeout);
    if (ret >= 0) {
        return ret;
    }
    switch(errno) {
        case EINTR:
        case EAGAIN:
            return 0;
        default:
            WARNING("epoll_wait return %d with errno[%d]", ret, errno);
            ret = -1;
    };
    return ret;
}

int Epoller::updateEvent(SocketPtr socket) {
    struct epoll_event event;
    int sockfd = socket->fd();
    event.data.ptr = (void*)socket;
    event.events = socket->getEvents();
    // TODO: let getEvent readable
    NOTICE("socket[%d] update event[%d] in epoll", sockfd, socket->getEvents());
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_MOD, sockfd, &event);
    if( ret < 0 ){
        FATAL("ctl socket:%d into epoll fd:%d failed errno:%d %s", sockfd, _epoll_socket, errno, strerror(errno));
        return -1;
    }
    return 0;
}

//int Epoller::updateEvent(SocketPtr socket) {
//  Poller::assertInLoopThread();
//  const int stat = socket->stat();
//  // new 和 deleted状态都表示epoll没有在监听，区别在于map是否建立
//  if (stat == kNew || stat == kDeleted) {
//    socket->set_stat(kAdded);
//    update(EPOLL_CTL_ADD, socket);
//  } else { //stat == kAdded 
//    if (socket->isNoneEvent()) {
//      update(EPOLL_CTL_DEL, socket);
//      socket->set_stat(kDeleted);
//    } else {
//      update(EPOLL_CTL_MOD, socket);
//    }
//  }
//  return 0;
//}

}
