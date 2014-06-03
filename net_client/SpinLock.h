
#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

#include "common.h"
#include "log.h"


namespace dyc {

class SpinLock : boost::noncopyable {
 public:
  SpinLock()
    : _holder(0) {
    MCHECK(pthread_spin_init(&mSpinLock, PTHREAD_PROCESS_PRIVATE));
  }

  ~SpinLock() {
    assert(_holder == 0);
    MCHECK(pthread_spin_destroy(&mSpinLock));
  }

  // must be called when locked, i.e. for assertion
  bool isLockedByThisThread() const {
    return _holder == pthread_self();
  }

  void assertLocked() const {
    assert(isLockedByThisThread());
  }

  // internal usage

  void lock() {
    MCHECK(pthread_spin_lock(&mSpinLock));
    _holder = pthread_self();
  }

  void unlock() {
    _holder = 0;
    MCHECK(pthread_spin_unlock(&mSpinLock));
  }

  pthread_spinlock_t* getPthreadMutex() 
  {
    return &mSpinLock;
  }

 private:
  pthread_spinlock_t mSpinLock;
  pthread_t _holder;

  class UnassignGuard : boost::noncopyable {
   public:
    UnassignGuard(SpinLock& owner)
      : _owner(owner)
    {
      _owner._holder = 0;
    }

    ~UnassignGuard()
    {
      _owner._holder = pthread_self();
    }

   private:
    SpinLock& _owner;
  }; // UnassignGuard

};


}

#endif  // SPINLOCK_H
