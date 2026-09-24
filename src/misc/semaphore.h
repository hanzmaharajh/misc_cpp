#pragma once

#include <condition_variable>
#include <tuple>
#include <type_traits>

namespace misc {

/// @brief A counting semaphore whose units are a divisible resource rather
/// than a slot count, so a caller may acquire several units at once.
/// @tparam Mutex The mutex type guarding the count
/// @tparam CondVar The condition variable type waiters block on
template <typename Mutex = std::mutex,
          typename CondVar = std::condition_variable>
class semaphore {
 public:
  /// @brief Constructs a semaphore with the given capacity
  /// @param size The total number of units available
  explicit semaphore(size_t size) : m_capacity(size) {}

  /// @brief Acquires @p size units, blocking until they are available
  /// @param size The number of units to take
  /// @note A request larger than the capacity still succeeds once the
  /// semaphore is completely free, rather than blocking forever.
  void acquire(size_t size = 1) {
    std::unique_lock lock(m_mutex);
    m_cv.wait(lock, [&] { return is_space_available(size); });
    m_in_use += size;
  }

  /// @brief Acquires @p size units if they are available right now
  /// @param size The number of units to take
  /// @return true if the units were acquired
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

  /// @brief Returns @p size units and wakes any waiters
  /// @param size The number of units to return. It must match what was
  /// acquired.
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

/// @brief Acquires units from a semaphore for the lifetime of the object
/// @tparam Semaphore The semaphore type
template <typename Semaphore>
class semaphore_lock {
  Semaphore& m_sem;
  size_t m_num;

 public:
  /// @brief Acquires @p num units, blocking until they are available
  /// @param sem The semaphore to take from. It must outlive the lock.
  /// @param num The number of units to take
  [[nodiscard]] explicit semaphore_lock(Semaphore& sem, size_t num)
      : m_sem(sem), m_num(num) {
    m_sem.acquire(m_num);
  }

  /// @brief Not copyable. The units are released exactly once.
  semaphore_lock(const semaphore_lock&) = delete;

  /// @copydoc semaphore_lock(const semaphore_lock&)
  semaphore_lock& operator=(const semaphore_lock&) = delete;

  /// @brief Releases the acquired units
  ~semaphore_lock() { m_sem.release(m_num); }
};

}  // namespace misc