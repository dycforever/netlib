
#include <boost/bind.hpp>
#include <signal.h>

#include "EventLoop.h"
#include "Mutex.h"
#include "Connection.h"
#include "Epoller.h"


namespace dyc {
__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop(Epoller* poller)
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
          Connection* connection = (Connection*)_active_events[i].data.ptr;
//          int fd = connection->fd();
//          if (connection == NULL) {
//              FATAL("find connection for fd[%d] failed", fd);
//          }
//          NOTICE("handle event in connection[%d]", fd);
          int ret = connection->handle(_active_events[i]);
          // TODO need use macro
          if (ret == Connection::CONN_REMOVE) {
              DEBUG("will remove conn");
              _poller->removeEvent(connection);
          } else if (ret == Connection::CONN_UPDATE) {
              _poller->updateEvent(connection);
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

int EventLoop::updateConnection(ConnectionPtr connection) {
    return _poller->updateEvent(connection);
}

int EventLoop::remove(ConnectionPtr connection) {
    return _poller->removeEvent(connection);
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
