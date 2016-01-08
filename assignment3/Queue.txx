#ifndef QUEUE_TXX
#define QUEUE_TXX

#include "Guard.hxx"

template<typename T>
SQueue<T>::SQueue (size_t size)
  : max_size_ (size), m_(*(new Mutex())), c_(*(new Condition(m_))),
      cFull_(*(new Condition(m_)))
{
}

template<typename T>
SQueue<T>::~SQueue (void)
{
}

template<typename T>
void
SQueue<T>::enqueue (const T &item, unsigned long timeout)
{
  m_.lock();    // locks the resource
  while (queue_.size () == max_size_)
    {
      cFull_.wait(timeout);     // blocks up to specified timeout if queue is full
    }
  this->queue_.push (item);
  c_.broadcast();   // wakes all waiters on empty condition
  m_.unlock();  // unlocks the resource
}

template<typename T>
T
SQueue<T>::dequeue (size_t timeout)
{
  m_.lock();    // locks the resource
  while (queue_.empty ())
    {
      c_.wait(timeout);     // blocks up to specified timeout if queue is empty
    }

  T tmp (this->queue_.front ());
  this->queue_.pop ();
  
  cFull_.broadcast();   // wakes all waiters on full condition
  m_.unlock();  // unlocks resource
  
  return tmp;
}


template<typename T>
size_t
SQueue<T>::current_size (void)
{
  return this->queue_.size ();
}


#endif
