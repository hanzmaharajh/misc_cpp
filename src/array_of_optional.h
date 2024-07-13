#pragma once

#include <array>
#include <bitset>
#include <cmath>
#include <iterator>
#include <memory>
#include <type_traits>

namespace misc {

template <typename T, size_t N>
class ArrayOfOptional {
 protected:
  std::bitset<N> is_set{};
  std::aligned_storage_t<sizeof(T), alignof(T[])> storage[N];

  T* data() { return reinterpret_cast<T*>(storage); }

  template <typename Array>
  void copy_to_empty_arr(Array&& o) {
    for (size_t i = 0; i < size(); ++i) {
      if (o.is_set[i]) {
        if constexpr (std::is_rvalue_reference_v<decltype(o)>) {
          new (data() + i) T(std::move(*o[i]));
        } else {
          new (data() + i) T(*o[i]);
        }
      }
    }
    is_set = o.is_set;
  }

 public:
  struct const_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const T*;
    using pointer = const T**;
    using reference = const T*;

    const_iterator(size_t index, const ArrayOfOptional& arro)
        : ind(index), arr(arro) {}

    reference operator*() const { return arr[ind]; }
    pointer operator->() { return &arr[ind]; }

    const_iterator& operator++() {
      ++ind;
      return *this;
    }
    const_iterator operator++(int) const {
      const auto retval = *this;
      ++retval;
      return retval;
    }
    const_iterator& operator--() {
      --ind;
      return *this;
    }
    const_iterator operator--(int) const {
      const auto retval = *this;
      --retval;
      return retval;
    }
    const_iterator operator+=(difference_type diff) {
      ind += diff;
      return *this;
    }
    const_iterator operator-=(difference_type diff) { return (*this += -diff); }

    friend bool operator==(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    friend bool operator!=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    const ArrayOfOptional& arr;
  };

  ArrayOfOptional() noexcept = default;

  ArrayOfOptional(const ArrayOfOptional& o) noexcept { copy_to_empty_arr(o); }

  ArrayOfOptional(ArrayOfOptional&& o) noexcept {
    copy_to_empty_arr(std::move(o));
  }

  ArrayOfOptional& operator=(const ArrayOfOptional& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(o);
    }
    return *this;
  }

  ArrayOfOptional& operator=(ArrayOfOptional&& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(std::move(o));
    }
    return *this;
  }

  ~ArrayOfOptional() { clear(); }

  [[nodiscard]] friend bool operator==(const ArrayOfOptional& lhs,
                                       const ArrayOfOptional& rhs) {
    if (lhs.is_set != rhs.is_set) {
      return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
      if (const auto l_ptr = lhs[i]) {
        if (const auto r_ptr = rhs[i]; *l_ptr != *r_ptr) {
          return false;
        }
      }
    }
    return true;
  }

  [[nodiscard]] friend bool operator!=(const ArrayOfOptional& lhs,
                                       const ArrayOfOptional& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] T* operator[](size_t pos) {
    return is_set[pos] ? data() + pos : nullptr;
  }

  [[nodiscard]] const T* operator[](size_t pos) const {
    return const_cast<ArrayOfOptional*>(this)->operator[](pos);
  }

  template <typename... Args>
  T& emplace(size_t pos, Args&&... args) {
    auto* ptr = data() + pos;
    if (is_set[pos]) {
      std::destroy_at(ptr);
    }
    is_set[pos] = true;
    return *new (ptr) T(std::forward<Args>(args)...);
  }

  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace(i, t);
    }
  }

  size_t erase(size_t pos) {
    if (auto&& s = is_set[pos]) {
      std::destroy_at(data() + pos);
      s = false;
      return 1;
    }
    return 0;
  }

  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      erase(i);
    }
  }

  [[nodiscard]] constexpr size_t size() const { return N; }
  const const_iterator begin() const { return const_iterator(0, *this); }
  const const_iterator end() const { return const_iterator(N, *this); }
};

}  // namespace misc
