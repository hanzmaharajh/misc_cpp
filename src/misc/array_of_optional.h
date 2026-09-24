#pragma once

#include <array>
#include <bitset>
#include <cmath>
#include <iterator>
#include <memory>
#include <type_traits>

namespace misc {

/// @brief A fixed-size array of N slots, each of which either holds a T or
/// is empty -- roughly a std::array<std::optional<T>, N>, but with the
/// "is set" flags packed into a bitset alongside one contiguous block of
/// storage rather than one flag per element.
/// @tparam T The element type
/// @tparam N The number of slots
template <typename T, size_t N>
class array_of_optional {
 protected:
  std::bitset<N> is_set{};
  std::aligned_storage_t<sizeof(T), alignof(T)> storage[N];

  [[nodiscard]] T* data() { return reinterpret_cast<T*>(storage); }
  [[nodiscard]] const T* data() const {
    return reinterpret_cast<const T*>(storage);
  }

  template <typename Array>
  void copy_to_empty_arr(Array&& o) {
    for (size_t i = 0; i < size(); ++i) {
      if (auto* ptr = o[i]) {
        if constexpr (std::is_rvalue_reference_v<decltype(o)>) {
          new (data() + i) T(std::move(*ptr));
        } else {
          new (data() + i) T(*ptr);
        }
      }
    }
    is_set = o.is_set;
  }

 public:
  /// @brief Iterates over every slot of the array, engaged or not.
  ///
  /// Dereferencing yields a `const T*` that is null for a slot that holds no
  /// value, in the same spirit as `array_of_optional::operator[]`, so reaching
  /// a member reads `(*it)->member`.
  ///
  /// @note There is deliberately no `operator->`. The standard requires
  /// `it->m` to mean `(*it).m`; here `*it` is itself a pointer, so an
  /// `operator->` could only mean `(*it)->m` -- quietly one level of
  /// indirection off, and skipping the null check that `*it` invites.
  struct const_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const T*;

    /// @brief Present so that std::iterator_traits is complete, which the
    /// standard helpers (std::next, std::prev, std::reverse_iterator) need.
    /// There is no operator->; @c operator* already yields this type.
    using pointer = const T*;

    using reference = const T*;

    /// @brief Constructs an iterator onto one slot
    /// @param index The slot index
    /// @param arro The container iterated over. It must outlive the iterator.
    const_iterator(size_t index, const array_of_optional& arro)
        : ind(index), arr(arro) {}

    /// @brief Returns a handle to the current slot's value
    /// @return A pointer to the value, or nullptr if the slot is empty
    reference operator*() const { return arr[ind]; }

    /// @brief Returns a handle to the slot @p diff positions away
    /// @param diff The offset from this iterator
    /// @return A pointer to the value, or nullptr if that slot is empty
    reference operator[](difference_type diff) const { return *(*this + diff); }

    /// @brief Returns the iterator @p diff positions forward
    /// @param diff How far to advance
    /// @return The advanced iterator
    const_iterator operator+(difference_type diff) const {
      auto retval = *this;
      retval += diff;
      return retval;
    }
    /// @brief Advances to the next slot, engaged or not
    /// @return *this
    const_iterator& operator++() {
      ++ind;
      return *this;
    }
    /// @brief Advances to the next slot, engaged or not
    /// @return The iterator's value from before the increment
    const_iterator operator++(int) {
      auto retval = *this;
      ++*this;
      return retval;
    }
    /// @brief Returns the iterator @p diff positions back
    /// @param diff How far to retreat
    /// @return The retreated iterator
    const_iterator operator-(difference_type diff) const {
      auto retval = *this;
      retval -= diff;
      return retval;
    }
    /// @brief Steps back to the previous slot, engaged or not
    /// @return *this
    const_iterator& operator--() {
      --ind;
      return *this;
    }
    /// @brief Steps back to the previous slot, engaged or not
    /// @return The iterator's value from before the decrement
    const_iterator operator--(int) {
      auto retval = *this;
      --*this;
      return retval;
    }
    /// @brief Advances this iterator by @p diff positions
    /// @param diff How far to advance
    /// @return *this
    const_iterator& operator+=(difference_type diff) {
      ind = static_cast<size_t>(static_cast<difference_type>(ind) + diff);
      return *this;
    }
    /// @brief Moves this iterator back by @p diff positions
    /// @param diff How far to retreat
    /// @return *this
    const_iterator& operator-=(difference_type diff) {
      return (*this += -diff);
    }

    /// @brief Returns the iterator @p lhs positions forward of @p rhs
    /// @param lhs How far to advance
    /// @param rhs The iterator to advance
    /// @return The advanced iterator
    friend const_iterator operator+(difference_type lhs,
                                    const const_iterator& rhs) {
      return rhs + lhs;
    }

    /// @brief Returns the distance between two iterators
    /// @param lhs The iterator to measure from
    /// @param rhs The iterator to measure to
    /// @return How many positions @p rhs is behind @p lhs
    friend difference_type operator-(const const_iterator& lhs,
                                     const const_iterator& rhs) {
      return static_cast<difference_type>(lhs.ind) -
             static_cast<difference_type>(rhs.ind);
    }

    /// @brief Compares position and container identity
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if both refer to the same slot of the same container
    friend bool operator==(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    /// @copydoc operator==()
    friend bool operator!=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs == rhs);
    };
    /// @brief Orders two iterators into the same container by position
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if @p lhs refers to an earlier slot than @p rhs
    friend bool operator<(const const_iterator& lhs,
                          const const_iterator& rhs) {
      return lhs.ind < rhs.ind;
    }
    /// @copydoc operator<()
    friend bool operator>(const const_iterator& lhs,
                          const const_iterator& rhs) {
      return rhs < lhs;
    }
    /// @copydoc operator<()
    friend bool operator<=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(rhs < lhs);
    }
    /// @copydoc operator<()
    friend bool operator>=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs < rhs);
    }

   private:
    size_t ind;
    const array_of_optional& arr;
  };

  /// @brief Constructs an array with every slot empty
  array_of_optional() noexcept = default;

  /// @brief Copies every engaged slot of @p o, leaving the rest empty
  /// @param o The array to copy
  array_of_optional(const array_of_optional& o) noexcept {
    copy_to_empty_arr(o);
  }

  /// @brief Moves every engaged slot of @p o, leaving the rest empty
  /// @param o The array to move from. Its slots stay engaged, holding
  /// moved-from values.
  array_of_optional(array_of_optional&& o) noexcept {
    copy_to_empty_arr(std::move(o));
  }

  /// @brief Replaces the contents with a copy of @p o
  /// @param o The array to copy
  /// @return *this
  array_of_optional& operator=(const array_of_optional& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(o);
    }
    return *this;
  }

  /// @brief Replaces the contents with the values moved out of @p o
  /// @param o The array to move from
  /// @return *this
  array_of_optional& operator=(array_of_optional&& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(std::move(o));
    }
    return *this;
  }

  /// @brief Destroys every engaged slot
  ~array_of_optional() { clear(); }

  /// @brief Exchanges contents with @p o
  /// @param o The array to exchange with
  /// @note The elements are moved rather than the raw storage bytes being
  /// exchanged, since T need not be trivially relocatable.
  void swap(array_of_optional& o) noexcept {
    array_of_optional tmp(std::move(*this));
    *this = std::move(o);
    o = std::move(tmp);
  }

  /// @brief Exchanges the contents of @p lhs and @p rhs
  /// @param lhs The first array
  /// @param rhs The second array
  friend void swap(array_of_optional& lhs, array_of_optional& rhs) noexcept {
    lhs.swap(rhs);
  }

  /// @brief Compares which slots are engaged and the values they hold
  /// @param lhs The first array
  /// @param rhs The second array
  /// @return true if the same slots are engaged and their values compare
  /// equal
  [[nodiscard]] friend bool operator==(const array_of_optional& lhs,
                                       const array_of_optional& rhs) {
    if (lhs.is_set != rhs.is_set) {
      return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
      if (const auto l_ptr = lhs[i]) {
        if (*l_ptr != *(rhs.data() + i)) {
          return false;
        }
      }
    }
    return true;
  }

  /// @copydoc operator==()
  [[nodiscard]] friend bool operator!=(const array_of_optional& lhs,
                                       const array_of_optional& rhs) {
    return !(lhs == rhs);
  }

  /// @brief Returns a handle to the value at @p pos
  /// @param pos The slot index. It must be less than size().
  /// @return A pointer to the value, or nullptr if the slot is empty
  [[nodiscard]] T* operator[](size_t pos) {
    return is_set[pos] ? data() + pos : nullptr;
  }

  /// @copydoc operator[](size_t)
  [[nodiscard]] const T* operator[](size_t pos) const {
    return const_cast<array_of_optional*>(this)->operator[](pos);
  }

  /// @brief Constructs a value in place at @p pos, destroying whatever was
  /// there
  /// @tparam ...Args The constructor argument types
  /// @param pos The slot index. It must be less than size().
  /// @param ...args The arguments forwarded to T's constructor
  /// @return A reference to the new value
  template <typename... Args>
  T& emplace(size_t pos, Args&&... args) {
    auto* ptr = data() + pos;
    if (is_set[pos]) {
      std::destroy_at(ptr);
    }
    is_set[pos] = true;
    return *new (ptr) T(std::forward<Args>(args)...);
  }

  /// @brief Assigns a copy of @p t to every slot, engaging all of them
  /// @param t The value to copy into each slot
  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace(i, t);
    }
  }

  /// @brief Empties the slot at @p pos, destroying any value it held
  /// @param pos The slot index. It must be less than size().
  /// @return 1 if a value was destroyed, 0 if the slot was already empty
  size_t erase(size_t pos) {
    if (auto&& s = is_set[pos]) {
      std::destroy_at(data() + pos);
      s = false;
      return 1;
    }
    return 0;
  }

  /// @brief Empties every slot, destroying any values they held
  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      erase(i);
    }
  }

  /// @brief Returns the number of slots, engaged or not
  /// @return N
  [[nodiscard]] constexpr size_t size() const { return N; }

  /// @brief Returns an iterator to the first slot
  /// @return An iterator over every slot, engaged or not
  [[nodiscard]] const_iterator begin() const {
    return const_iterator(0, *this);
  }

  /// @brief Returns an iterator past the last slot
  /// @return The end iterator
  [[nodiscard]] const_iterator end() const { return const_iterator(N, *this); }

  /// @copydoc begin()
  [[nodiscard]] const_iterator cbegin() const { return begin(); }

  /// @copydoc end()
  [[nodiscard]] const_iterator cend() const { return end(); }
};

}  // namespace misc
