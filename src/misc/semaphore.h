#pragma once

#include <condition_variable>
#include <tuple>
#include <type_traits>

namespace misc {

template <typename Mutex = std::mutex,
          typename CondVar = std::condition_variable>
class semaphore {
 public:
  explicit semaphore(size_t size) : m_capacity(size) {}

  void acquire(size_t size = 1) {
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [&] { return is_space_available(size); });
    m_in_use += size;
  }

  bool try_acquire(size_t size = 1) {
    std::unique_lock lock(m_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
      if (is_space_available(size)) {
        m_in_use += size;
        return true;
      }
    }
    return false;
  }

  void release(size_t size) {
    std::unique_lock lock(m_mutex);
    m_in_use -= size;
    m_cv.notify_all();
  }

 protected:
  bool is_space_available(size_t size) const {
    // If no resources have been acquired, let this through.
    // Otherwise, we may block forever waiting for a capacity less than the
    // requested size.
    if (m_in_use == 0) return true;

    return m_in_use + size <= m_capacity;
  }

  size_t m_capacity;
  size_t m_in_use{};

  Mutex m_mutex;
  CondVar m_cv;
};

// RAII acquire/release
template <typename Semaphore>
class semaphore_lock {
  Semaphore& m_sem;
  size_t m_num;

 public:
  [[nodiscard]] explicit semaphore_lock(Semaphore& sem, size_t num)
      : m_sem(sem), m_num(num) {
    m_sem.acquire(m_num);
  }

  ~semaphore_lock() { m_sem.release(m_num); }
};

}  // namespace misc