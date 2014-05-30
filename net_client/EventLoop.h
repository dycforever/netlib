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

class Connection;
class Epoller;

class EventLoop
{
 public:
    typedef boost::function<void()> DelayFunctor;
    typedef Connection* ConnectionPtr;
    typedef struct epoll_event Event;

    EventLoop( boost::shared_ptr<Epoller> );
    ~EventLoop();  // force out-line dtor, for scoped_ptr members.

    void loop();
    void quit();

    int64_t iteration() const { return iteration_; }

    void runInLoop(const DelayFunctor& cb);
    void queueInLoop(const DelayFunctor& cb);

    // internal usage
    int updateConnection(ConnectionPtr);
    void removeConnection(ConnectionPtr );

    bool inThisThread() {return _threadId == pthread_self();}
    static EventLoop* getEventLoopOfCurrentThread();
    int remove(ConnectionPtr);

private:
    Event* _active_events;
    void callPendingFunctors();

    typedef std::vector<ConnectionPtr> ConnectionList;

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
