#ifndef GUARD_HXX
#define GUARD_HXX

#include "Mutex.hxx"

template <typename T>
class Guard
{
public:
  Guard (T &mutex, bool locked = false);

  ~Guard (void);

  void acquire (void);

  void release (void);

  bool locked (void);

private:
  Mutex &m_;
};

#include "Guard.txx"

#endif
