
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <sys/epoll.h>

namespace dyc {

class Channel {
public:

    static const int CONN_REMOVE = -1;
    static const int CONN_CONTINUE = 0;
    static const int CONN_UPDATE = 1;

    virtual int handle(const epoll_event& event) = 0;
    virtual int fd() = 0;
    virtual int getEvents() = 0;
    virtual void setEvents(int) = 0;
};

}

#endif
