#include "Condition.hxx"
#include <iostream>
Condition::Condition (Mutex &mutex):m_(mutex), cond_(PTHREAD_COND_INITIALIZER)
// Initializes condition variable
{
}


Condition::~Condition (void)
{
  pthread_cond_destroy(&cond_);
}


void
Condition::wait (unsigned long timeout)
{
  // converts unsigned long to timespec and waits for the specified timeout
  const timespec &t = convert(timeout);
  m_.lock();
  int tracker = pthread_cond_timedwait(&cond_, &m_.m_, &t);
  m_.unlock();
  
  // throws exception of timeout value is exceeded
  if(!tracker) {
    throw timeout;
  }
}


void
Condition::wait (std::function <bool (void)> condition, unsigned long timeout)
{
  // waits for the specified timeout for the given condition to return true
  int tracker;
  const timespec &t = convert(timeout);
  m_.lock();
  while(!condition()) {
    tracker = pthread_cond_timedwait(&cond_, &m_.m_, &t);
  }
  m_.unlock();
  
  // throws exception of timeout value is exceeded
  if(!tracker) {
    throw timeout;
  }
}


void
Condition::wait (const timespec &t)
{
  // waits on the mutex for the specified time value
  m_.lock();
  pthread_cond_timedwait(&cond_, &m_.m_, &t);
  m_.unlock();
}


timespec
Condition::convert (unsigned long ms)
{
  // converts from relative timeout to absolute timestamp
  timespec abs, cur;
  clock_gettime(CLOCK_REALTIME, &cur);
  abs.tv_sec = ms / 1000 - cur.tv_sec;
  abs.tv_nsec = (ms % 1000) * 1000000 - cur.tv_nsec;
  return abs;
}


void
Condition::signal (void)
{
  // wakes a single waiter on the cond var
  pthread_cond_signal(&cond_);
}


void
Condition::broadcast (void)
{
  // wakes all waiters on the cond var
  pthread_cond_broadcast(&cond_);
}
