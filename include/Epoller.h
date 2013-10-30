#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <boost/shared_ptr.hpp>

#include "Socket.h"
#include "common.h"

namespace dyc {


class Epoller {
public:
    typedef Socket* SocketPtr;
    typedef struct epoll_event Event;

    Epoller();
    ~Epoller() {pthread_mutex_destroy(&_mutex);}
    int createEpoll();
    void setTimeout(int);

    int addRead(SocketPtr);
    int addWrite(SocketPtr);
    int addRW(SocketPtr);
    int removeEvent(SocketPtr);

    int updateEvent(SocketPtr);
    int poll(Event*);

    static const int EPOLL_MAX_LISTEN_NUMBER=500;

private:
    void lock();
    void unlock();
    int _removeEvent(SocketPtr);
    int _addEvent(SocketPtr, uint32_t);

    pthread_mutex_t _mutex;
    int _epoll_socket;
    int _timeout;
};

inline void* epollRun(void* p) {
    return p;
}


//class EpollTask : public Task {
//public:
//    EpollTask(Epoller* poller) {
//        _param = reinterpret_cast<void*>(poller);
//    }
//
//    void* run(void* p) {
//        Epoller* poller = reinterpret_cast<Epoller*>(p);
//        poller->loop();
//        return NULL;
//    }
//};

}
#endif
