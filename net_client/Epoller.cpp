#include <boost/bind.hpp>
#include "Epoller.h"

namespace dyc {

Epoller::Epoller() {
    _timeout = 1000;
}


void Epoller::setTimeout(int t) {
    _timeout = t;
}

int Epoller::addRead(ChannelPtr channel) {
    lock();
    int ret = addEvent(channel, EPOLLIN);
    unlock();
    return ret;
}

int Epoller::addWrite(ChannelPtr channel) {
    lock();
    int ret = addEvent(channel, EPOLLOUT);
    unlock();
    return ret;
}

int Epoller::addRW(ChannelPtr channel) {
    int ret = 0;
    ret = addEvent(channel, EPOLLIN|EPOLLOUT);
    return ret;
}

int Epoller::removeEvent(ChannelPtr channel) {
    lock();
    int ret = _removeEvent(channel);
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

int Epoller::_removeEvent(ChannelPtr channel) {
    int sockfd = channel->fd();
    int epsfd = _epoll_socket;
    DEBUG("del port in epoll %d", epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_DEL, sockfd, NULL);
    if( ret < 0 ){
        FATAL("remove channel:%d from epoll fd:%d failed", sockfd, epsfd);
        return -1;
    }
    return 0;
}

int Epoller::addEvent(ChannelPtr channel, uint32_t op_types) {
    int sockfd = channel->fd();
    channel->setEvents(op_types);

    struct epoll_event event;
    event.data.ptr = (void*)channel;
    event.events = op_types;

    int epsfd = _epoll_socket;
    const char* ev = (op_types==EPOLLIN) ? "epoll_in" : "epoll_out";
    DEBUG("add socket[%d] event[%s] in epoll socket[%d]", sockfd, ev, epsfd);
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_ADD, sockfd, &event);
    if( ret < 0 ){
        FATAL("add channel:%d into epoll fd:%d failed errno:%d %s", sockfd, epsfd, errno, strerror(errno));
        return -1;
    }
    return 0;
}

int Epoller::poll(Event* list) {
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

int Epoller::updateEvent(ChannelPtr channel) {
    struct epoll_event event;
    int sockfd = channel->fd();
    event.data.ptr = (void*)channel;
    event.events = channel->getEvents();
    DEBUG("channel[%d] update event[%d] in epoll", sockfd, channel->getEvents());
    int ret = epoll_ctl(_epoll_socket, EPOLL_CTL_MOD, sockfd, &event);
    if( ret < 0 ){
        FATAL("ctl channel:%d into epoll fd:%d failed errno:%d %s", sockfd, _epoll_socket, errno, strerror(errno));
        return -1;
    }
    return 0;
}

}
