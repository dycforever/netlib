#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <boost/shared_ptr.hpp>

#include "common.h"

#include "Channel.h"
#include "netutils/Log.h"

namespace dyc {

class Epoller {
public:
    typedef Channel* ChannelPtr;
    typedef struct epoll_event Event;

    Epoller();
    ~Epoller() {pthread_mutex_destroy(&_mutex);}
    int createEpoll();
    void setTimeout(int);

    int addRead(ChannelPtr);
    int addWrite(ChannelPtr);
    int addRW(ChannelPtr);
    int removeEvent(ChannelPtr);

    int updateEvent(ChannelPtr);
    int poll(Event*);

    std::string eventsToStr(uint32_t);

    static const int EPOLL_MAX_LISTEN_NUMBER=500;

private:
    void lock();
    void unlock();
    int _removeEvent(ChannelPtr);
    int addEvent(ChannelPtr, uint32_t);

    pthread_mutex_t _mutex;
    int _epoll_socket;
    int _timeout;
};

inline void* epollRun(void* p) {
    return p;
}

}
#endif
