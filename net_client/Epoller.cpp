#include <boost/bind.hpp>
#include "Epoller.h"

namespace dyc {

Epoller::Epoller() {
    _timeout = 100;
}


void Epoller::setTimeout(int t) {
    _timeout = t;
}

int Epoller::addRead(ConnectionPtr connection) {
    lock();
    int ret = _addEvent(connection, EPOLLIN);
    unlock();
    return ret;
}

int Epoller::addWrite(ConnectionPtr connection) {
    lock();
    int ret = _addEvent(connection, EPOLLOUT);
    unlock();
    return ret;
}

int Epoller::addRW(ConnectionPtr connection) {
    int ret = 0;
    ret = _addEvent(connection, EPOLLIN|EPOLLOUT);
    return ret;
}

int Epoller::removeEvent(ConnectionPtr connection) {
    lock();
    int ret = _removeEvent(connection);
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
    DEBUG("create epoll socket[%d]", sock);
    _epoll_socket = sock;

    return 0;
}

int Epoller::_removeEvent(ConnectionPtr socket) {
    int sockfd = socket->fd();
    int epsfd = _epoll_socket;
    DEBUG("del port in epoll %d", epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_DEL, sockfd, NULL);
    if( ret < 0 ){
        FATAL("remove socket:%d from epoll fd:%d failed", sockfd, epsfd);
        return -1;
    }
    return 0;
}

int Epoller::_addEvent(ConnectionPtr socket, uint32_t op_types) {
    int sockfd = socket->fd();
    socket->setEvents(op_types);

    struct epoll_event event;
    event.data.ptr = (void*)socket;
    event.events = op_types;

    int epsfd = _epoll_socket;
    const char* ev = (op_types==EPOLLIN) ? "epoll_in" : "epoll_out";
    DEBUG("add [%d][%s] event in epoll socket[%d]", sockfd, ev, epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_ADD, sockfd, &event);
    if( ret < 0 ){
        FATAL("add socket:%d into epoll fd:%d failed errno:%d %s", sockfd, epsfd, errno, strerror(errno));
        return -1;
    }
    return 0;
}

int Epoller::poll(Event* list) {
    DEBUG("epoll wait on socket[%d] size:%d", _epoll_socket, 10);
    int ret = epoll_wait(_epoll_socket, list, EPOLL_MAX_LISTEN_NUMBER, _timeout);
    if (ret >= 0) {
        return ret;
    }
    switch(errno) {
        case EINTR:
        case EAGAIN:
            return 0;
        default:
            WARN("epoll_wait return %d with errno[%d]: %s", ret, errno, strerror(errno));
            ret = -1;
    };
    return ret;
}

int Epoller::updateEvent(ConnectionPtr socket) {
    struct epoll_event event;
    int sockfd = socket->fd();
    event.data.ptr = (void*)socket;
    event.events = socket->getEvents();
    // TODO: let getEvent readable
    DEBUG("socket[%d] update event[%d] in epoll", sockfd, socket->getEvents());
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_MOD, sockfd, &event);
    if( ret < 0 ){
        FATAL("ctl socket:%d into epoll fd:%d failed errno:%d %s", sockfd, _epoll_socket, errno, strerror(errno));
        return -1;
    }
    return 0;
}

}
