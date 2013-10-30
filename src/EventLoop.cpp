
#include <boost/bind.hpp>
#include <signal.h>

#include "EventLoop.h"
#include "Mutex.h"
#include "Socket.h"
#include "Epoller.h"


namespace dyc {
__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop(boost::shared_ptr<Epoller> poller)
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    iteration_(0),
    _poller(poller) {
    if (t_loopInThisThread)
    {
      FATAL("Another EventLoop exists in this thread ");
    }
    else
    {
      t_loopInThisThread = this;
    }
    _active_events = NEW Event[Epoller::EPOLL_MAX_LISTEN_NUMBER];
    if (_active_events == NULL) {
        FATAL("new active events failed");
    }
}

EventLoop::~EventLoop()
{
  t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
//    assertInLoopThread();
    looping_ = true;
    quit_ = false;
  
    while (!quit_)
    {
      sleep(1);
      ++iteration_;
  
      int nfds = _poller->poll(_active_events);
      NOTICE("eventloop detect %d events", nfds);
      if (nfds < 0) {
          FATAL("poll failed: %d", nfds);
          return;
      }
  
      eventHandling_ = true;
      for(int i = 0; i < nfds; ++i) {
          // TODO dangerous ?!
          Socket* socket = (Socket*)_active_events[i].data.ptr;
          int fd = socket->fd();
          if (socket == NULL) {
              FATAL("find socket for fd[%d] failed", fd);
          //    continue;
          }
          NOTICE("handle event in socket[%d]", fd);
          int ret = socket->handle(_active_events[i]);
          if (ret != 0) {
              WARNING("handle failed");
              _poller->removeEvent(socket);
          }
      }
  
      eventHandling_ = false;
      callPendingFunctors();
    }
  
    looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
}

int EventLoop::updateSocket(SocketPtr socket) {
    return _poller->updateEvent(socket);
}

int EventLoop::remove(SocketPtr socket) {
    return _poller->removeEvent(socket);
}

void EventLoop::runInLoop(const DelayFunctor& cb)
{
  if (inThisThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const DelayFunctor& cb)
{
  {
  MutexLockGuard lock(_mutex);
  _waitQueue.push_back(cb);
  }

}

void EventLoop::callPendingFunctors()
{
    MutexLockGuard lock(_mutex);
    std::vector<DelayFunctor>::iterator iter;
    for (iter = _waitQueue.begin(); iter != _waitQueue.end(); ++iter) {
        (*iter)();
    }
}

}
