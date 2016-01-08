#ifndef GUARD_TXX
#define GUARD_TXX

template<typename T>
Guard<T>::Guard (T &mutex, bool locked):m_(mutex)
{
  // Locks or unlocks mutex depending on flag
  if (locked)
    acquire();
  else
    release();
}

template<typename T>
Guard<T>::~Guard (void)
{
  // Ensures mutex is unlocked when Guard goes out of scope
  release();
}

template<typename T>
void
Guard<T>::acquire (void)
{
  // Locks mutex
  m_.lock();
}


template<typename T>
void
Guard<T>::release (void)
{
  // Unlocks mutex
  m_.unlock();
}

template<typename T>
bool
Guard<T>::locked (void)
{
  // Returns locked state of mutex
  m_.locked();
}

#endif
