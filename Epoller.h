#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <boost/shared_ptr.hpp>
#include <list>

#include "common.h"

#include "Channel.h"
#include "netutils/Log.h"

namespace dyc {

class Epoller {
public:
    typedef Channel* ChannelPtr;
    typedef std::list<ChannelPtr> ChannelList;
    typedef ChannelList::iterator ChannelListIter;
    typedef struct epoll_event Event;

    Epoller();
    ~Epoller();
    int createEpoll();
    void setTimeout(int);

    int poll(Event*);
    int addRead(ChannelPtr);
    int addWrite(ChannelPtr);
    int addRW(ChannelPtr);
    int removeEvent(ChannelPtr);
    int updateEvent(ChannelPtr);

    static const int EPOLL_MAX_LISTEN_NUMBER=500;

private:
    int removeEventFromEpoll(ChannelPtr);
    int addEvent(ChannelPtr, uint32_t);

    std::string eventsToStr(uint32_t);

private:
    int mEpollSocket;
    int mTimeout;
    ChannelList mChannelList;
};

inline void* epollRun(void* p) {
    return p;
}

}
#endif
