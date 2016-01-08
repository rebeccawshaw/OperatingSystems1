#ifndef QUEUE_HXX
#define QUEUE_HXX

#include <queue>
#include "Mutex.hxx"
#include "Condition.hxx"

/**
 * @brief Synchronized wrapper around STL queue.
 */
template<typename T>
class SQueue
{
public:
  /**
   * Constructs an SQueue that will hold *at most* size elements.
   */
  SQueue (size_t size);

  ~SQueue (void);

  /**
   * @brief Adds an item to the queue.
   * @details Will block up to the specified timeout if the queue is full.
   * @param item The item to be placed on the queue.
   * @param timeout A relative timeout in milliseconds.
   */
  void enqueue (const T &item, unsigned long timeout = 0);

  /**
   * @brief Removes an item from the queue.
   * @details Will block up to the specified timeout if the queue is empty.
   * @param timeout A relative timeout in milliseconds.
   * @return The item that was removed.
   */
  T dequeue (size_t timeout = 0);

  /**
   * @brief Returns the current size of the queue.
   */
  size_t current_size (void);

private:
  std::queue<T> queue_;
  const size_t max_size_;

  // Fill in here
  Mutex &m_;
  Condition &c_;
  Condition &cFull_;
};

#include "Queue.txx"
#endif
