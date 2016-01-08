#include <iostream>
#include "Task.hxx"
#include "Guard.hxx"
#include <string.h>

template<typename T, typename MUTEX>
Task<T, MUTEX>::Task (void)
{
}

template<typename T, typename MUTEX>
Task<T, MUTEX>::~Task (void)
{
  // cancels all the threads
  cancel();
}

template<typename T, typename MUTEX>
void
Task<T, MUTEX>::activate (size_t number, bool detached)
{
  Guard<MUTEX> g(mutex_, true);
  // determines thread attribute in either detached or not detached state
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  
  if (detached)
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
  // activates the specified number of threads
  for (size_t i = 0; i < number; i++) {
    pthread_t t;
    pthread_create(&t, &attr, thread_func, this);
    threads_.push_back(t);
  }
}

template<typename T, typename MUTEX>
void *
Task<T, MUTEX>::thread_func (void *handle)
{
  // invokes svc on obj that created the thread
  T *inv = new T((reinterpret_cast<Task *>(handle)) -> svc());
  return inv;
}

template<typename T, typename MUTEX>
std::list<T>
Task<T, MUTEX>::wait (void)
{
  Guard<MUTEX> g(mutex_, true);
  // waits for all attached threads to complete
  T *retval;
  std::list<T> values;
  std::list<pthread_t>::const_iterator i;
  for (i = threads_.begin(); i != threads_.end(); i++) {
    pthread_join(*i, &retval);
    values.push_back(*retval);
  }
  return values;
}

template<typename T, typename MUTEX>
void
Task<T, MUTEX>::cancel (void)
{
  Guard<MUTEX> g(mutex_, true);
  // cancels all threads in the thread_ list
  std::list<pthread_t>::const_iterator i;
  for (i = threads_.begin(); i != threads_.end(); i++) {
    pthread_cancel(*i);
  }
}
