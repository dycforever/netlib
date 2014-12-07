#include <boost/bind.hpp>
#include "Epoller.h"

namespace dyc {

Epoller::Epoller() : mTimeout(-1) {
    mTimeout = 1000;
}

Epoller::~Epoller() {
    ChannelListIter iter = mChannelList.begin();
    for (; iter != mChannelList.end();) {
        removeEventFromEpoll(*iter);
        mChannelList.erase(iter++);
    }
    int ret = ::close(mEpollSocket);
//    int ret  = 0;
    if (ret == 0) {
        INFO_LOG("close epoll fd %d success", mEpollSocket);
    } else {
        FATAL_LOG("close epoll fd %d failed: %d %s", mEpollSocket, errno, strerror(errno));
    }
}

void Epoller::setTimeout(int t) {
    mTimeout = t;
}

int Epoller::addRead(ChannelPtr channel) {
    return addEvent(channel, EPOLLIN);
}

int Epoller::addWrite(ChannelPtr channel) {
    return addEvent(channel, EPOLLOUT);
}

int Epoller::addRW(ChannelPtr channel) {
    return addEvent(channel, EPOLLIN|EPOLLOUT);
}

int Epoller::removeEvent(ChannelPtr channel) {
    mChannelList.remove(channel);
    return removeEventFromEpoll(channel);
}

int Epoller::createEpoll() {
    int sock = epoll_create(EPOLL_MAX_LISTEN_NUMBER);
    if (sock < 0) {
        return sock;
    }
    DEBUG_LOG("create epoll socket[%d]", sock);
    mEpollSocket = sock;
    return 0;
}

int Epoller::removeEventFromEpoll(ChannelPtr channel) {
    int sockfd = channel->fd();
    DEBUG_LOG("del port in epoll %d", mEpollSocket);
    int ret = epoll_ctl(mEpollSocket, EPOLL_CTL_DEL, sockfd, NULL);
    if( ret < 0 ){
        FATAL_LOG("remove channel:%d from epoll fd:%d failed", sockfd, mEpollSocket);
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

    const char* ev = (op_types==EPOLLIN) ? "epoll_in" : "epoll_out";
    DEBUG_LOG("add socket[%d] event[%s] in epoll socket[%d]", sockfd, ev, mEpollSocket);
    int ret = epoll_ctl(mEpollSocket, EPOLL_CTL_ADD, sockfd, &event);
    if( ret < 0 ){
        FATAL_LOG("add channel:%d into epoll fd:%d failed errno:%d %s", sockfd, mEpollSocket, errno, strerror(errno));
        return -1;
    }
    mChannelList.push_back(channel);
    return 0;
}

int Epoller::poll(Event* list) {
    int ret = epoll_wait(mEpollSocket, list, EPOLL_MAX_LISTEN_NUMBER, mTimeout);
    if (ret >= 0) {
        return ret;
    }
    switch(errno) {
        case EINTR:
        case EAGAIN:
            return 0;
        default:
            WARN_LOG("epoll_wait return %d with errno[%d]: %s", ret, errno, strerror(errno));
            ret = -1;
    };
    sleep(1);
    return ret;
}

std::string Epoller::eventsToStr(uint32_t event)
{
    std::string str;
    if (event & EPOLLIN) {
        str += "EPOLLIN";
    }
    if (event & EPOLLOUT) {
        str += "|EPOLLOUT";
    }
    return str;
}

int Epoller::updateEvent(ChannelPtr channel) {
    struct epoll_event event;
    int sockfd = channel->fd();
    event.data.ptr = (void*)channel;
    event.events = channel->getEvents();
    DEBUG_LOG("channel[%d] update event[%s] in epoll", sockfd, eventsToStr(event.events).c_str());
    int ret = epoll_ctl(mEpollSocket, EPOLL_CTL_MOD, sockfd, &event);
    if( ret < 0 ){
        FATAL_LOG("ctl channel:%d into epoll fd:%d failed errno:%d %s", sockfd, mEpollSocket, errno, strerror(errno));
        return -1;
    }
    return 0;
}

}
