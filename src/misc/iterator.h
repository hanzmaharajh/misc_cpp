#pragma once

#include <iterator>
#include <memory>
#include <tuple>
#include <utility>

namespace misc {

/// @brief Storage for the result of an @c operator-> on an iterator whose
/// @c operator* returns a proxy object by value.
///
/// @c operator-> is applied repeatedly until it yields a raw pointer, so an
/// iterator that has no real object to point at can return one of these: the
/// proxy is kept alive inside it for as long as the full expression that
/// wrote @c it-> lasts.
template <typename T>
struct arrow_proxy {
  /// @brief The proxied object, kept alive for the arrow expression.
  T value;

  /// @brief Yields a pointer to the proxied object.
  T* operator->() { return &value; }

  /// @copydoc operator->()
  const T* operator->() const { return &value; }
};

/// @brief Deduces the proxied type from the object passed in
template <typename T>
arrow_proxy(T) -> arrow_proxy<T>;

/// @brief An output iterator that appends to a container, up to a limit.
///
/// Once @p limit elements have been written, further assignments are
/// silently discarded.
/// @tparam Container A container supporting emplace_back()
template <typename Container>
class limit_back_insert_iterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using container_type = Container;

 private:
  size_t lim;
  container_type& container;

 public:
  /// @brief Constructs an iterator appending at most @p limit elements
  /// @param limit The maximum number of elements to append
  /// @param c The container to append to. It must outlive the iterator.
  limit_back_insert_iterator(size_t limit, Container& c)
      : lim(limit), container(c) {}

  /// @brief Appends @p value, unless the limit has been reached
  /// @param value The value to append
  /// @return *this
  limit_back_insert_iterator& operator=(
      const typename Container::value_type& value) {
    if (lim) {
      container.emplace_back(value);
      --lim;
    }
    return *this;
  }

  /// @copydoc operator=(const typename Container::value_type&)
  limit_back_insert_iterator& operator=(
      typename Container::value_type&& value) {
    if (lim) {
      container.emplace_back(std::move(value));
      --lim;
    }
    return *this;
  }

  /// @brief No-op. Assignment is what appends, so this returns *this.
  /// @return *this
  limit_back_insert_iterator& operator*() { return *this; }

  /// @brief No-op. Appending already advances the container.
  /// @return *this
  limit_back_insert_iterator& operator++() { return *this; }

  /// @copydoc operator++()
  limit_back_insert_iterator operator++(int) { return *this; }
};

/// @brief An output iterator over [begin, end) that silently discards
/// anything written to it once @c end is reached.
///
/// @tparam Iter The underlying output iterator type
/// @note Assignments past the end are dropped rather than written to @c *end.
/// That's why @c operator* hands back a proxy instead of a @c value_type& --
/// at the end of the range there is no element to hand out a reference to.
template <typename Iter>
class limit_output_iterator {
 public:
  /// @brief What @c operator* yields: an assignment target that writes
  /// through to the underlying iterator, or discards, depending on whether
  /// the end of the range has been reached.
  class proxy {
    Iter it;
    bool writable;

   public:
    /// @brief Constructs a proxy for one position
    /// @param i The position written through to
    /// @param w Whether writes should reach @p i at all
    proxy(Iter i, bool w) : it(i), writable(w) {}

    /// @brief Writes @p value through to the underlying iterator, or
    /// discards it if the end of the range has been passed
    /// @param value The value to write
    /// @return *this
    template <typename U>
    proxy& operator=(U&& value) {
      if (writable) *it = std::forward<U>(value);
      return *this;
    }
  };

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = typename std::iterator_traits<Iter>::difference_type;
  using pointer = void;
  using reference = proxy;

 private:
  Iter begin;
  Iter end;

 public:
  /// @brief Constructs an iterator writing over [begin_it, end_it)
  /// @param begin_it The first position to write to
  /// @param end_it One past the last position that may be written to
  limit_output_iterator(Iter begin_it, Iter end_it)
      : begin(begin_it), end(end_it) {}

  /// @brief Constructs an already-exhausted iterator, which discards
  /// everything written to it
  /// @param end_it The end of the (empty) range
  explicit limit_output_iterator(Iter end_it)
      : limit_output_iterator(end_it, end_it) {}

  /// @brief Compares both the current and the end position
  /// @param rhs The iterator to compare against
  /// @return true if both iterators are at the same point of the same range
  bool operator==(const limit_output_iterator& rhs) const {
    return begin == rhs.begin && end == rhs.end;
  }

  /// @copydoc operator==()
  bool operator!=(const limit_output_iterator& rhs) const {
    return !(*this == rhs);
  }

  /// @brief Returns an assignment target for the current position
  /// @return A proxy that writes through, or discards if the end has been
  /// reached
  proxy operator*() const { return proxy{begin, begin != end}; }

  /// @brief Advances to the next position, stopping at the end
  /// @return *this
  limit_output_iterator& operator++() {
    if (begin != end) ++begin;
    return *this;
  }

  /// @brief Advances to the next position, stopping at the end
  /// @return The iterator's value from before the increment
  limit_output_iterator operator++(int) {
    auto retval = *this;
    ++*this;
    return retval;
  }
};

/// @brief A forward iterator that walks several ranges as if they were one.
///
/// The ranges are given as a flat pack of begin/end pairs. A trailing begin
/// with no matching end is treated as unbounded.
/// @tparam ...Iters The iterator type of each begin and end
template <typename... Iters>
class chain_iterator {
  using tuple_type = std::tuple<Iters...>;
  tuple_type iters;
  using first_iter_type = std::tuple_element_t<0, tuple_type>;

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename std::iterator_traits<first_iter_type>::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = typename std::iterator_traits<first_iter_type>::pointer;
  using reference = typename std::iterator_traits<first_iter_type>::reference;

 private:
  template <size_t N>
  value_type& prv_value() {
    auto& it = std::get<N>(iters);
    if constexpr (N < sizeof...(Iters) - 2) {
      auto& end_it = std::get<N + 1>(iters);
      if (it == end_it) {
        return prv_value<N + 2>();
      }
    }
    return *it;
  }

  template <size_t N>
  const value_type& prv_value() const {
    const auto& it = std::get<N>(iters);
    if constexpr (N < sizeof...(Iters) - 2) {
      const auto& end_it = std::get<N + 1>(iters);
      if (it == end_it) {
        return prv_value<N + 2>();
      }
    }
    return *it;
  }

  template <size_t N>
  void prv_inc() {
    auto& it = std::get<N>(iters);
    if constexpr (N < sizeof...(Iters) - 2) {
      auto& end_it = std::get<N + 1>(iters);
      if (it == end_it) {
        prv_inc<N + 2>();
        return;
      }
    }
    ++it;
  }

 public:
  /// @brief Constructs an iterator over the given begin/end pairs
  /// @param ...iterators The begin and end of each range, in order
  chain_iterator(Iters&&... iterators)
      : iters{std::forward<Iters>(iterators)...} {}

  /// @brief Compares the position within every chained range
  /// @param rhs The iterator to compare against
  /// @return true if both iterators are at the same position
  bool operator==(const chain_iterator& rhs) const {
    return iters == rhs.iters;
  }

  /// @copydoc operator==()
  bool operator!=(const chain_iterator& rhs) const { return !(*this == rhs); }

  /// @brief Returns the element at the current position, which lives in the
  /// first range that has not been exhausted
  /// @return A reference to the element
  value_type& operator*() { return prv_value<0>(); }

  /// @copydoc operator*()
  const value_type& operator*() const { return prv_value<0>(); }

  /// @brief Returns a pointer to the element at the current position
  /// @return A pointer to the element
  value_type* operator->() { return std::addressof(prv_value<0>()); }

  /// @copydoc operator->()
  const value_type* operator->() const {
    return std::addressof(prv_value<0>());
  }

  /// @brief Advances within the current range, moving on to the next range
  /// once this one is exhausted
  /// @return *this
  chain_iterator& operator++() {
    prv_inc<0>();
    return *this;
  }

  /// @brief Advances to the next element
  /// @return The iterator's value from before the increment
  chain_iterator operator++(int) {
    chain_iterator old = *this;
    ++*this;
    return old;
  }
};

}  // namespace misc