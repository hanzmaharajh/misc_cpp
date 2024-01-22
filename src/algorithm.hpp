#pragma once

#include <algorithm>
#include <boost/algorithm/apply_permutation.hpp>
#include <boost/scope_exit.hpp>
#include <numeric>

#include "allocated_arrays.hpp"

namespace misc {

/// @brief Sorts elements in the input range by comparing their values after
/// being transformed. The transform will only be called once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
void sort_cache_key(Iter begin, Iter end, UnaryOperation op, Compare comp) {
  using Diff = typename std::iterator_traits<Iter>::difference_type;
  using T = typename std::iterator_traits<Iter>::value_type;
  using Key = decltype(op(std::declval<T>()));

  const Diff length = std::distance(begin, end);

  allocated_arrays_storage<Key, Diff> arrs(length, length);

  auto keys_range = arrs.template get<0>();
  auto end_key_created = keys_range.begin();

  BOOST_SCOPE_EXIT_ALL(&) {
    std::destroy(keys_range.begin(), end_key_created);
  };

  std::for_each(begin, end,
                [&](const auto& v) { new (end_key_created++) Key{op(v)}; });

  auto indices_range = arrs.template get<1>();

  std::iota(indices_range.begin(), indices_range.end(), Diff{});
  std::sort(indices_range.begin(), indices_range.end(), [&](Diff a, Diff b) {
    return comp(keys_range[static_cast<size_t>(a)],
                keys_range[static_cast<size_t>(b)]);
  });
  boost::algorithm::apply_permutation(begin, end, indices_range.begin(),
                                      indices_range.end());

  std::destroy(indices_range.begin(), indices_range.end());
}

/// @brief Sorts elements in the input range by comparing their values after
/// being transformed. The transform will only be called once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
template <typename Iter, typename UnaryOperation>
void sort_cache_key(Iter first, Iter last, UnaryOperation op) {
  sort_cache_key(first, last, op, std::less<>{});
}

/// @brief Finds the min element in the input range by comparing the range's
/// values after being transformed. The transform will only be called once per
/// element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
Iter min_element_cache_key(Iter first, Iter last, UnaryOperation op,
                           Compare comp) {
  if (first == last) return last;

  auto min_val = op(*first);
  auto min_it = first;
  while (++first != last) {
    auto val = op(*first);
    if (comp(val, min_val)) {
      min_val = std::move(val);
      min_it = first;
    }
  }
  return min_it;
}

/// @brief Finds the min element in the input range by comparing the range's
/// values after being transformed. The transform will only be called once per
/// element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
template <typename Iter, typename UnaryOperation>
Iter min_element_cache_key(Iter first, Iter last, UnaryOperation op) {
  return min_element_cache_key(first, last, op, std::less<>{});
}

/// @brief Finds the min and max elements in the input range by comparing the
/// range's values after being transformed. The transform will only be called
/// once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
auto minmax_element_cache_key(Iter first, Iter last, UnaryOperation op,
                              Compare comp) {
  if (first == last) return std::pair{last, last};

  auto min_val = op(*first);
  auto max_val = min_val;
  auto min_it = first;
  auto max_it = first;
  while (++first != last) {
    auto val = op(*first);

    if (comp(val, min_val)) {
      min_val = val;
      min_it = first;
    }

    if (comp(max_val, val)) {
      max_val = std::move(val);
      max_it = first;
    }
  }
  return std::pair{min_it, max_it};
}

/// @brief Finds the min and max elements in the input range by comparing the
/// range's values after being transformed. The transform will only be called
/// once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
template <typename Iter, typename UnaryOperation>
auto minmax_element_cache_key(Iter first, Iter last, UnaryOperation op) {
  return minmax_element_cache_key(first, last, op, std::less<>{});
}
}  // namespace misc
