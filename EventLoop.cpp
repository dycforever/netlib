
#include <boost/bind.hpp>
#include <signal.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Epoller.h"

namespace dyc {
__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop(Epoller* poller)
    : looping_(false),
    quit_(false),
    eventHandling_(false),
    iteration_(0),
    _poller(poller) {
        if (t_loopInThisThread) {
            FATAL_LOG("Another EventLoop exists in this thread ");
        } else {
            t_loopInThisThread = this;
        }
        _active_events = NEW Event[Epoller::EPOLL_MAX_LISTEN_NUMBER];
        if (_active_events == NULL) {
            FATAL_LOG("new active events failed");
        }
    }

EventLoop::~EventLoop() {
    DELETES(_active_events);
    t_loopInThisThread = NULL;
}

void EventLoop::loop() {
    assert(!looping_);
    _threadId = pthread_self();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        ++iteration_;

        int nfds = _poller->poll(_active_events);
        DEBUG_LOG("eventloop detect %d events", nfds);
        if (nfds < 0) {
            FATAL_LOG("poll failed: %d", nfds);
            return;
        }

        eventHandling_ = true;
        for(int i = 0; i < nfds; ++i) {
            Channel* connection = (Channel*)_active_events[i].data.ptr;
            assert(connection != NULL);
            int ret = connection->handle(_active_events[i]);
            if (ret == Channel::CONN_REMOVE) {
                DEBUG_LOG("will remove conn");
                _poller->removeEvent(connection);
            } else if (ret == Channel::CONN_UPDATE) {
                _poller->updateEvent(connection);
            }
        }

        eventHandling_ = false;
        callPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
}

int EventLoop::updateChannel(ChannelPtr connection) {
    return _poller->updateEvent(connection);
}

int EventLoop::remove(ChannelPtr connection) {
    return _poller->removeEvent(connection);
}

void EventLoop::runInLoop(const DelayFunctor& cb) {
    if (inThisThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const DelayFunctor& cb) {
    LockGuard<MutexLock> lock(mLock);
    _waitQueue.push_back(cb);
}

void EventLoop::callPendingFunctors() {
    std::vector<DelayFunctor> functors;
    {
        LockGuard<MutexLock> lock(mLock);
        if (_waitQueue.size() == 0) {
            return;
        }
        functors.swap(_waitQueue);
    }
    DEBUG_LOG("calling PendingFunctors: %lu", functors.size());
    std::vector<DelayFunctor>::iterator iter;
    for (iter = functors.begin(); iter != functors.end(); ++iter) {
        (*iter)();
    }
}

}
