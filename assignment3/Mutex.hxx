#ifndef MUTEX_HXX
#define MUTEX_HXX

#include <stdexcept>
#include <system_error>
#include <algorithm>
#include <pthread.h>

class timeout : public std::runtime_error
{
public:
  timeout (const std::string &what) : std::runtime_error (what) {}
};

class Condition;

class Mutex
{
friend class Condition;
public:

  // Initializes mutex
  Mutex (void)
    : locked_ (false), m_(PTHREAD_MUTEX_INITIALIZER)
  {
  }

  Mutex (const Mutex &) = delete;
  
  // Locks mutex only if previously unlocked
  void lock (void)
  {
    if (!locked_) {
        pthread_mutex_lock(&m_);
        locked_ = true;
    }
  }

  // Unlocks mutex only if previously locked
  void unlock (void)
  {
    if (locked_) {
        pthread_mutex_unlock(&m_);
        locked_ = false;
    }
  }

  // Returns locked state of mutex
  bool locked (void)
  {
    return locked_;
  }

private:
  bool locked_;
  pthread_mutex_t m_;
};

#endif
