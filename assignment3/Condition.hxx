#ifndef CONDITION_HXX
#define CONDITION_HXX

#include <utility>
#include <time.h>
#include "Mutex.hxx"

class Condition
{
public:
  Condition (Mutex &mutex);

  ~Condition (void);

  Condition (const Condition &) = delete;

  /**
   * @brief Waits on the mutex up to the specified relative timeout.
   * @details If timeout is zero, it waits indefinately.
   * @pre The associated mutex is locked.
   * @post The associated mutex is locked.
   * @throw timeout if the timeout value is exceeded.
   */
  void wait (unsigned long timeout = 0);

  /**
   * @brief Waits on the mutex up to the specified timeout
   * for the specified condition function to return true.
   * @details If timeout is zero, it waits indefinately.
   * @pre The associated mutex is locked.
   * @post The associated mutex is locked.
   * @throw timeout if the timeout value is exceeded.
   */
  void wait (std::function <bool (void)> condition, unsigned long timeout = 0);

  /**
   * Wakes a single waiter on the condition variable.
   */
  void signal (void);

  /**
   * Wakes all waiters on the condition variable.
   */
  void broadcast (void);

private:
  /// Convert a relaive timeout into an absolute timestamp
  timespec convert (unsigned long ms);

  void wait (const timespec &ts);

  // You may need to fill in here
  Mutex &m_;
  pthread_cond_t cond_;
};

#endif
