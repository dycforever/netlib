#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "Mutex.h"
#include "Epoller.h"

namespace dyc 
{

class Socket;
class Epoller;

class EventLoop
{
 public:
    typedef boost::function<void()> DelayFunctor;
    typedef Socket* SocketPtr;
    typedef struct epoll_event Event;

    EventLoop( boost::shared_ptr<Epoller> );
    ~EventLoop();  // force out-line dtor, for scoped_ptr members.

    void loop();
    void quit();

    int64_t iteration() const { return iteration_; }

    void runInLoop(const DelayFunctor& cb);
    void queueInLoop(const DelayFunctor& cb);

    // internal usage
    int updateSocket(SocketPtr);
    void removeSocket(SocketPtr );

    bool inThisThread() {return _threadId == pthread_self();}
    static EventLoop* getEventLoopOfCurrentThread();
    int remove(SocketPtr);

private:
    Event* _active_events;
    void callPendingFunctors();

    void printActiveSockets() const; // DEBUG

    typedef std::vector<SocketPtr> SocketList;

    bool looping_; /* atomic */
    bool quit_; /* atomic */
    bool eventHandling_; /* atomic */
    int64_t iteration_;
    pthread_t _threadId;
    boost::shared_ptr<Epoller> _poller;

    MutexLock _mutex;
    std::vector<DelayFunctor> _waitQueue;
};

}
#endif  // EVENTLOOP_H
