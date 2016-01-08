#ifndef TASK_HXX
#define TASK_HXX

#include <list>
#include <thread>

#include "Mutex.hxx"

/**
 * @class Task
 * @brief Abstract base class for an classes that have their own thread of control.
 *
 */
template<typename T, typename MUTEX = Mutex>
class Task
{
public:
  Task (void);

  virtual ~Task (void);

  /// Derived classes will put their business logic here.
  virtual T svc (void) = 0;

  /// Activates a specified number of threads in either detached or
  /// not detached state.
  void activate (size_t number, bool detached);

  /// This should wait for all threads attached to this task to complete,
  /// and return a list of their return values.
  std::list<T> wait (void);

  void cancel (void);
private:

  /// This is a function that you can pass to pthread_create.
  /// It should turn around and invoke svc on the object
  /// that created the thread.
  static void * thread_func (void *);

  std::list< pthread_t > threads_;

  MUTEX mutex_;
};

#include "Task.txx"

#endif
