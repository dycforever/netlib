#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <boost/shared_ptr.hpp>

#include "Connection.h"

namespace dyc {

class Epoller {
public:
    typedef Connection* ConnectionPtr;
    typedef struct epoll_event Event;

    Epoller();
    ~Epoller() {pthread_mutex_destroy(&_mutex);}
    int createEpoll();
    void setTimeout(int);

    int addRead(ConnectionPtr);
    int addWrite(ConnectionPtr);
    int addRW(ConnectionPtr);
    int removeEvent(ConnectionPtr);

    int updateEvent(ConnectionPtr);
    int poll(Event*);

    static const int EPOLL_MAX_LISTEN_NUMBER=500;

private:
    void lock();
    void unlock();
    int _removeEvent(ConnectionPtr);
    int _addEvent(ConnectionPtr, uint32_t);

    pthread_mutex_t _mutex;
    int _epoll_socket;
    int _timeout;
};

inline void* epollRun(void* p) {
    return p;
}

}
#endif
