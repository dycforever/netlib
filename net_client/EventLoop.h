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

class Channel;
class Epoller;

class EventLoop
{
 public:
    typedef boost::function<void()> DelayFunctor;
    typedef Channel* ChannelPtr;
    typedef struct epoll_event Event;

//    EventLoop( boost::shared_ptr<Epoller> );
    EventLoop( Epoller* );
    ~EventLoop();  // force out-line dtor, for scoped_ptr members.

    void loop();
    void quit();

    int64_t iteration() const { return iteration_; }

    void runInLoop(const DelayFunctor& cb);
    void queueInLoop(const DelayFunctor& cb);

    // internal usage
    int updateChannel(ChannelPtr);
    void removeChannel(ChannelPtr );

    bool inThisThread() {return _threadId == pthread_self();}
    void assertInLoopThread() {assert(_threadId == pthread_self());}
    static EventLoop* getEventLoopOfCurrentThread();
    int remove(ChannelPtr);

private:
    Event* _active_events;
    void callPendingFunctors();

    typedef std::vector<ChannelPtr> ChannelList;

    bool looping_; /* atomic */
    bool quit_; /* atomic */
    bool eventHandling_; /* atomic */
    int64_t iteration_;
    pthread_t _threadId;
//    boost::shared_ptr<Epoller> _poller;
    Epoller* _poller;

    MutexLock mLock;
    std::vector<DelayFunctor> _waitQueue;
};

}
#endif  // EVENTLOOP_H
