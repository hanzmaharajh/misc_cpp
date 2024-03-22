#pragma once

#include <algorithm>
#include <boost/algorithm/apply_permutation.hpp>
#include <boost/scope_exit.hpp>
#include <numeric>

#include "allocated_arrays.h"

namespace misc {

/// @brief Sorts elements in the input range by comparing their values after
/// being transformed. The transform will only be called once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
void transform_sort(Iter begin, Iter end, UnaryOperation op, Compare comp) {
  using Diff = typename std::iterator_traits<Iter>::difference_type;
  using T = typename std::iterator_traits<Iter>::value_type;
  using Key = decltype(op(std::declval<T>()));

  const Diff length = std::distance(begin, end);

  allocated_arrays_storage<Key, Diff> arrs(length, length);

  const auto& [keys_range, indices_range] = arrs;

  auto end_key_created = keys_range.begin();
  BOOST_SCOPE_EXIT_ALL(&, keys = keys_range) {
    std::destroy(keys.begin(), end_key_created);
  };

  std::for_each(begin, end,
                [&](const auto& v) { new (end_key_created++) Key{op(v)}; });

  std::iota(indices_range.begin(), indices_range.end(), Diff{});
  std::sort(indices_range.begin(), indices_range.end(),
            [&, keys = keys_range](Diff a, Diff b) {
              return comp(keys[static_cast<size_t>(a)],
                          keys[static_cast<size_t>(b)]);
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
void transform_sort(Iter first, Iter last, UnaryOperation op) {
  transform_sort(first, last, op, std::less<>{});
}

/// @brief Finds the min element in the input range by comparing the range's
/// values after being transformed. The transform will only be called once per
/// element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
[[nodiscard]] Iter transform_min_element(Iter first, Iter last,
                                         UnaryOperation op, Compare comp) {
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
[[nodiscard]] Iter transform_min_element(Iter first, Iter last,
                                         UnaryOperation op) {
  return transform_min_element(first, last, op, std::less<>{});
}

/// @brief Finds the min and max elements in the input range by comparing the
/// range's values after being transformed. The transform will only be called
/// once per element.
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
[[nodiscard]] std::pair<Iter, Iter> transform_minmax_element(Iter first,
                                                             Iter last,
                                                             UnaryOperation op,
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
[[nodiscard]] std::pair<Iter, Iter> transform_minmax_element(
    Iter first, Iter last, UnaryOperation op) {
  return transform_minmax_element(first, last, op, std::less<>{});
}

/// @brief Transforms elements if they satisfy some predicate
/// @param begin Beginning iterator
/// @param end End iterator
/// @param out Output operator
/// @param pred The predicate evaluated for each element in the range
/// @param op The transform applied to each element that satisfies the predicate
template <typename Iter, typename OIter, typename Predicate,
          typename UnaryOperation>
[[nodiscard]] Iter transform_if(Iter first, Iter last, OIter out,
                                Predicate pred, UnaryOperation op) {
  while (first != last) {
    const auto& v = *first;
    if (pred(v)) {
      *out++ = op(v);
    }
    ++first;
  }
  return first;
}

}  // namespace misc
