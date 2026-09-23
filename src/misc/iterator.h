#pragma once

#include <iterator>

namespace misc {
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
  limit_back_insert_iterator(size_t limit, Container& c)
      : lim(limit), container(c) {}

  limit_back_insert_iterator& operator=(
      const typename Container::value_type& value) {
    if (lim) {
      container.emplace_back(value);
      --lim;
    }
    return *this;
  }

  limit_back_insert_iterator& operator=(
      typename Container::value_type&& value) {
    if (lim) {
      container.emplace_back(std::move(value));
      --lim;
    }
    return *this;
  }

  limit_back_insert_iterator& operator*() { return *this; }

  limit_back_insert_iterator& operator++() { return *this; }
  limit_back_insert_iterator operator++(int) { return this; }
};

template <typename Iter>
class limit_output_iterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = typename std::iterator_traits<Iter>::value_type;
  using difference_type = typename std::iterator_traits<Iter>::difference_type;
  using pointer = typename std::iterator_traits<Iter>::pointer;
  using reference = typename std::iterator_traits<Iter>::reference;

 private:
  Iter begin;
  Iter end;

 public:
  limit_output_iterator(Iter begin_it, Iter end_it)
      : begin(begin_it), end(end_it) {}

  limit_output_iterator(Iter end_it) : limit_output_iterator(end_it, end_it) {}

  bool operator==(const limit_output_iterator& rhs) const {
    return begin == rhs.begin && end == rhs.end;
  }
  bool operator!=(const limit_output_iterator& rhs) const {
    return !(*this == rhs);
  }

  value_type& operator*() { return *begin; }

  limit_output_iterator& operator++() {
    if (begin != end) ++begin;
    return *this;
  }

  limit_output_iterator operator++(int) {
    if (begin != end) return limit_output_iterator{begin++, end};

    return *this;
  }
};

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
  chain_iterator(Iters&&... iterators)
      : iters{std::forward<Iters>(iterators)...} {}

  bool operator==(const chain_iterator& rhs) const {
    return iters == rhs.iters;
  }
  bool operator!=(const chain_iterator& rhs) const { return !(*this == rhs); }

  value_type& operator*() { return prv_value<0>(); }
  const value_type& operator*() const { return prv_value<0>(); }

  chain_iterator& operator++() {
    prv_inc<0>();
    return *this;
  }
  chain_iterator operator++(int) {
    chain_iterator old = *this;
    ++*this;
    return old;
  }
};

}  // namespace misc