#pragma once

#include <cassert>
#include <climits>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

#include "allocated_storages.h"
#include "compact_int_array.h"

namespace misc {

/// @brief A growable sequence of slots, each of which either holds a T or
/// is empty -- roughly a std::vector<std::optional<T>>, but with the "is
/// set" flags packed into a bitmap that shares a single allocation with the
/// elements rather than one flag per element.
/// @tparam T The element type
template <typename T>
class vector_of_optional {
 protected:
  size_t curr_size = 0;
  allocated_storages<uint8_t, T> storages;

  static constexpr size_t BITS_STORE_IND = 0;
  static constexpr size_t DATA_STORE_IND = 1;
  static allocated_storages<uint8_t, T> make_storages(size_t capacity) {
    allocated_storages<uint8_t, T> storages{
        (capacity + CHAR_BIT - 1) / CHAR_BIT, capacity};
    const auto& bits_range = storages.template get<BITS_STORE_IND>();
    std::fill(bits_range.begin(), bits_range.end(), 0x00);
    return storages;
  }

  T* data() {
    return reinterpret_cast<T*>(
        storages.template get<DATA_STORE_IND>().begin());
  }

  static bool is_bit_set(size_t i, const arr_range<uint8_t>& octs) {
    return misc::get_bit(octs.begin(), i);
  }

  bool is_set(size_t i) const {
    const auto& range = storages.template get<BITS_STORE_IND>();
    assert(i < range.size() * sizeof(uint8_t) * CHAR_BIT);
    return misc::get_bit(range.begin(), i);
  }

  void set_bit(size_t i) {
    const auto& range = storages.template get<BITS_STORE_IND>();
    assert(i < range.size() * sizeof(uint8_t) * CHAR_BIT);
    misc::set_bit(range.begin(), true, i);
  }

  void reset_bit(size_t i) {
    const auto& range = storages.template get<BITS_STORE_IND>();
    assert(i < range.size() * sizeof(uint8_t) * CHAR_BIT);
    misc::set_bit(range.begin(), false, i);
  }

  static size_t exp_reallocation_size(size_t s) {
    return std::max<size_t>(2 * (s - 1), 1);
  }

  void maybe_reallocate_and_copy_exact(size_t s) {
    auto tmp_storages = make_storages(s);

    this->copy_to_empty_storages(std::move(storages), tmp_storages);

    using std::swap;
    swap(storages, tmp_storages);
    destroy_arr_elements(tmp_storages);
  }

  void maybe_reallocate_and_copy(size_t s) {
    if (capacity() >= s) return;
    maybe_reallocate_and_copy_exact(exp_reallocation_size(s));
  }

  static void destroy_arr_elements(allocated_storages<uint8_t, T>& storages) {
    const auto& data_range = storages.template get<DATA_STORE_IND>();
    const auto& bits_range = storages.template get<BITS_STORE_IND>();
    for (size_t i = 0; i < data_range.size(); ++i) {
      if (is_bit_set(i, bits_range)) {
        std::destroy_at(data_range.begin() + i);
      }
    }

    std::destroy_n(bits_range.begin(), bits_range.size());
  }

  template <typename Array1, typename Array2>
  static void copy_to_empty_storages(Array1&& from, Array2&& to) {
    auto to_data_range = to.template get<DATA_STORE_IND>();
    auto from_data_range = from.template get<DATA_STORE_IND>();

    auto to_bits_range = to.template get<BITS_STORE_IND>();
    auto from_bits_range = from.template get<BITS_STORE_IND>();

    // `to` may be smaller than `from` (a shrinking reallocation), so both
    // loops have to stop at whichever range runs out first.
    const auto bits_to_copy =
        std::min(from_bits_range.size(), to_bits_range.size());
    std::copy_n(from_bits_range.begin(), bits_to_copy, to_bits_range.begin());

    const auto elements_to_copy =
        std::min(from_data_range.size(), to_data_range.size());
    for (size_t i = 0; i < elements_to_copy; ++i) {
      if (is_bit_set(i, from_bits_range)) {
        if constexpr (std::is_rvalue_reference_v<Array1&&>) {
          new (to_data_range.begin() + i) T(std::move(from_data_range[i]));
        } else {
          new (to_data_range.begin() + i) T(from_data_range[i]);
        }
      }
    }
  }

 public:
  /// @brief Iterates over every slot of the vector, engaged or not.
  ///
  /// Dereferencing yields a `const T*` that is null for a slot that holds no
  /// value, in the same spirit as `vector_of_optional::operator[]`, so
  /// reaching a member reads `(*it)->member`.
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
    /// @param v The container iterated over. It must outlive the iterator.
    const_iterator(size_t index, const vector_of_optional& v)
        : ind(index), arr(v) {}

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
    };

    /// @brief Returns the distance between two iterators
    /// @param lhs The iterator to measure from
    /// @param rhs The iterator to measure to
    /// @return How many positions @p rhs is behind @p lhs
    friend difference_type operator-(const const_iterator& lhs,
                                     const const_iterator& rhs) {
      return static_cast<difference_type>(lhs.ind) -
             static_cast<difference_type>(rhs.ind);
    };

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
    const vector_of_optional& arr;
  };

  /// @brief Constructs an empty vector with no allocation
  vector_of_optional() noexcept : curr_size{0}, storages{make_storages(0)} {}

  /// @brief Copies the size and contents of @p o, engaged slots and all
  /// @param o The vector to copy
  vector_of_optional(const vector_of_optional& o)
      : storages{make_storages(o.size())} {
    curr_size = o.size();
    for (size_t i = 0; i < o.size(); ++i) {
      if (const auto* v = o[i]) {
        emplace_at(i, *v);
      }
    }
  };

  /// @brief Takes over the storage of @p o
  /// @param o The vector to move from. It is left empty.
  vector_of_optional(vector_of_optional&& o) noexcept
      : storages{make_storages(0)} {
    swap(o);
  }

  /// @brief Replaces the contents with a copy of @p o
  /// @param o The vector to copy
  /// @return *this
  vector_of_optional& operator=(const vector_of_optional& o) {
    if (&o != this) {
      vector_of_optional tmp(o);
      swap(tmp);
    }
    return *this;
  }

  /// @brief Replaces the contents with the storage taken from @p o
  /// @param o The vector to move from. It is left empty.
  /// @return *this
  vector_of_optional& operator=(vector_of_optional&& o) noexcept {
    if (&o != this) {
      swap(o);
      o.clear();
    }
    return *this;
  }

  /// @brief Destroys every engaged slot and releases the storage
  ~vector_of_optional() { destroy_arr_elements(storages); };

  /// @brief Exchanges contents with @p o
  /// @param o The vector to exchange with
  void swap(vector_of_optional& o) noexcept {
    using std::swap;
    swap(o.storages, storages);
    swap(o.curr_size, curr_size);
  }

  /// @brief Compares size, which slots are engaged, and the values held
  /// @param lhs The first vector
  /// @param rhs The second vector
  /// @return true if both have the same size and hold equal values in the
  /// same slots
  /// @note Capacity plays no part in the comparison.
  [[nodiscard]] friend bool operator==(const vector_of_optional& lhs,
                                       const vector_of_optional& rhs) {
    if (lhs.size() != rhs.size()) return false;

    for (size_t i = 0; i < lhs.size(); ++i) {
      const auto* l_ptr = lhs[i];
      const auto* r_ptr = rhs[i];
      if ((l_ptr == nullptr) != (r_ptr == nullptr)) return false;
      if (l_ptr != nullptr && !(*l_ptr == *r_ptr)) return false;
    }
    return true;
  }

  /// @copydoc operator==()
  [[nodiscard]] friend bool operator!=(const vector_of_optional& lhs,
                                       const vector_of_optional& rhs) {
    return !(lhs == rhs);
  }

  /// @brief Returns a handle to the value at @p pos
  /// @param pos The slot index. It must be less than size().
  /// @return A pointer to the value, or nullptr if the slot is empty
  [[nodiscard]] T* operator[](size_t pos) {
    return is_set(pos) ? data() + pos : nullptr;
  }

  /// @copydoc operator[](size_t)
  [[nodiscard]] const T* operator[](size_t pos) const {
    return const_cast<vector_of_optional*>(this)->operator[](pos);
  }

  /// @brief Appends a copy of @p arg as a new engaged slot
  /// @param arg The value to append
  /// @return A pointer to the appended value
  T* push_back(const T& arg) { return emplace_back(arg); }

  /// @brief Appends @p arg as a new engaged slot
  /// @param arg The value to move in
  /// @return A pointer to the appended value
  T* push_back(T&& arg) { return emplace_back(std::move(arg)); }

  /// @brief Appends a value constructed in place as a new engaged slot
  /// @tparam ...Args The constructor argument types
  /// @param ...args The arguments forwarded to T's constructor
  /// @return A pointer to the new value
  template <typename... Args>
  T* emplace_back(Args&&... args) {
    maybe_reallocate_and_copy(size() + 1);
    return emplace_at(curr_size++, std::forward<Args>(args)...);
  }

  /// @brief Replaces the slot at @p pos with a value constructed in place
  /// @tparam ...Args The constructor argument types
  /// @param pos The slot index. It must be less than capacity().
  /// @param ...args The arguments forwarded to T's constructor
  /// @return A pointer to the new value
  /// @note This overwrites a slot; it does not shift anything, and it does
  /// not change size().
  template <typename... Args>
  T* emplace_at(size_t pos, Args&&... args) {
    reset(pos);
    set_bit(pos);
    return new (data() + pos) T(std::forward<Args>(args)...);
  }

  /// @brief Empties the slot at @p pos, destroying any value it held
  /// @param pos The slot index. It must be less than capacity().
  /// @return nullptr
  T* emplace_at(size_t pos, std::nullopt_t) {
    reset(pos);
    return nullptr;
  }

  /// @brief Inserts a value constructed in place at @p pos, shifting the
  /// slots at and after @p pos one position along
  /// @tparam ...Args The constructor argument types
  /// @param pos The index to insert at. It must not exceed size().
  /// @param ...args The arguments forwarded to T's constructor
  /// @return A pointer to the new value
  template <typename... Args>
  T* emplace(size_t pos, Args&&... args) {
    const auto s = size();
    if (s == pos) return emplace_back(std::forward<Args>(args)...);

    if (s + 1 <= capacity()) {
      // If we don't need to reallocate, we can just move the data behind the
      // new object
      for (size_t i = s; i > pos; --i) {
        if (is_set(i - 1)) {
          emplace_at(i, std::move(*(data() + i - 1)));
          reset(i - 1);
        } else {
          reset(i);
        }
      }
    } else {
      // We need to reallocate.
      // Copy the data to the new storage, with a hole for the new object
      auto tmp_storages = make_storages(exp_reallocation_size(s + 1));

      auto to_data_range = tmp_storages.template get<DATA_STORE_IND>();
      auto from_data_range = storages.template get<DATA_STORE_IND>();

      auto to_bits_range = tmp_storages.template get<BITS_STORE_IND>();
      auto from_bits_range = storages.template get<BITS_STORE_IND>();

      for (size_t i = 0; i < s; ++i) {
        const size_t to_pos = i + (i >= pos);
        bool is_set = is_bit_set(i, from_bits_range);
        misc::set_bit(to_bits_range.begin(), is_set, to_pos);
        if (is_set) {
          new (to_data_range.begin() + to_pos) T(std::move(from_data_range[i]));
        }
      }

      using std::swap;
      swap(storages, tmp_storages);
      destroy_arr_elements(tmp_storages);
    }

    auto retval = emplace_at(pos, std::forward<Args>(args)...);
    ++curr_size;
    return retval;
  }

  /// @brief Assigns a copy of @p t to every slot, engaging all of them
  /// @param t The value to copy into each slot
  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace_at(i, t);
    }
  }

  /// @brief Empties every slot, destroying any values they held
  /// @note Unlike clear(), this leaves size() unchanged.
  void fill(std::nullopt_t) {
    for (size_t i = 0; i < size(); ++i) {
      reset(i);
    }
  }

  /// @brief Empties the slot at @p pos, destroying any value it held
  /// @param pos The slot index. It must be less than capacity().
  void reset(size_t pos) {
    if (is_set(pos)) {
      reset_bit(pos);
      if (const auto* arr = data()) std::destroy_at(arr + pos);
    }
  }

  /// @brief Changes the number of slots to @p s
  /// @param s The new size. Slots added by growing start out empty; slots
  /// removed by shrinking have their values destroyed.
  void resize(size_t s) {
    if (s < size()) {
      for (size_t i = s; i < size(); ++i) {
        reset(i);
      }
      curr_size = s;
    } else if (s > size()) {
      reserve(s);
      curr_size = s;
    }
  }

  /// @brief Removes the slot at @p pos, shifting every later slot down one
  /// position
  /// @param pos The slot index. Out-of-range indices are ignored.
  void erase(size_t pos) {
    if (pos >= size()) return;

    reset(pos);
    for (size_t i = pos + 1; i < size(); ++i) {
      if (is_set(i)) {
        emplace_at(i - 1, std::move(*(data() + i)));
        reset(i);
      } else {
        reset(i - 1);
      }
    }
    --curr_size;
  }

  /// @brief Destroys every value and reduces the size to zero
  /// @note The capacity is left alone.
  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      reset(i);
    }
    curr_size = 0;
  }

  /// @brief Grows the capacity to at least @p s slots
  /// @param s The capacity to make room for. Smaller values are ignored.
  void reserve(size_t s) {
    if (capacity() >= s) return;
    maybe_reallocate_and_copy_exact(s);
  }

  /// @brief Returns the number of slots, engaged or not
  /// @return The current size
  [[nodiscard]] size_t size() const { return curr_size; }

  /// @brief Returns how many slots fit without reallocating
  /// @return The current capacity
  [[nodiscard]] size_t capacity() const {
    return storages.template get<DATA_STORE_IND>().size();
  }

  /// @brief Returns an iterator to the first slot
  /// @return An iterator over every slot, engaged or not
  [[nodiscard]] const_iterator begin() const {
    return const_iterator(0, *this);
  }

  /// @brief Returns an iterator past the last slot
  /// @return The end iterator
  [[nodiscard]] const_iterator end() const {
    return const_iterator(size(), *this);
  }
};

}  // namespace misc
