#pragma once

#include <algorithm>
#include <boost/algorithm/apply_permutation.hpp>
#include <boost/scope_exit.hpp>
#include <functional>
#include <memory>
#include <numeric>

#include "allocated_storages.h"

namespace misc {

/// @brief Sorts elements in the input range by comparing their values after
/// being transformed. The transform will only be called once per element.
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @tparam Compare The comparison function type
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
template <typename Iter, typename UnaryOperation, typename Compare>
void transform_sort(Iter begin, Iter end, UnaryOperation&& op, Compare&& comp) {
  using Diff = typename std::iterator_traits<Iter>::difference_type;
  using T = typename std::iterator_traits<Iter>::value_type;
  using Key = std::remove_cv_t<
      std::remove_reference_t<std::invoke_result_t<UnaryOperation&, T&>>>;

  const Diff length = std::distance(begin, end);

  allocated_storages<Key, Diff> arrs(length, length);

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
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @param first Beginning iterator
/// @param last End iterator
/// @param op The transform applied to each element in the range
template <typename Iter, typename UnaryOperation>
void transform_sort(Iter first, Iter last, UnaryOperation&& op) {
  transform_sort(first, last, std::forward<UnaryOperation>(op), std::less<>{});
}

/// @brief Finds the min element in the input range by comparing the range's
/// values after being transformed. The transform will only be called once per
/// element.
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @tparam Compare The comparison function type
/// @param first Beginning iterator
/// @param last End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
/// @return An iterator to the minimum element, or @p last if the range is
/// empty
template <typename Iter, typename UnaryOperation, typename Compare>
[[nodiscard]] Iter transform_min_element(Iter first, Iter last,
                                         UnaryOperation&& op, Compare&& comp) {
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
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @param first Beginning iterator
/// @param last End iterator
/// @param op The transform applied to each element in the range
/// @return An iterator to the minimum element, or @p last if the range is
/// empty
template <typename Iter, typename UnaryOperation>
[[nodiscard]] Iter transform_min_element(Iter first, Iter last,
                                         UnaryOperation&& op) {
  return transform_min_element(first, last, std::forward<UnaryOperation>(op),
                               std::less<>{});
}

/// @brief Finds the min and max elements in the input range by comparing the
/// range's values after being transformed. The transform will only be called
/// once per element.
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @tparam Compare The comparison function type
/// @param first Beginning iterator
/// @param last End iterator
/// @param op The transform applied to each element in the range
/// @param comp The comparison function
/// @return Iterators to the minimum and maximum elements, both @p last if
/// the range is empty
template <typename Iter, typename UnaryOperation, typename Compare>
[[nodiscard]] std::pair<Iter, Iter> transform_minmax_element(
    Iter first, Iter last, UnaryOperation&& op, Compare&& comp) {
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
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @param first Beginning iterator
/// @param last End iterator
/// @param op The transform applied to each element in the range
/// @return Iterators to the minimum and maximum elements, both @p last if
/// the range is empty
template <typename Iter, typename UnaryOperation>
[[nodiscard]] std::pair<Iter, Iter> transform_minmax_element(
    Iter first, Iter last, UnaryOperation&& op) {
  return transform_minmax_element(first, last, std::forward<UnaryOperation>(op),
                                  std::less<>{});
}

/// @brief Transforms elements if they satisfy some predicate
/// @tparam Iter The input range's iterator type
/// @tparam OIter The output iterator type
/// @tparam Predicate The predicate type
/// @tparam UnaryOperation The transform type
/// @param first Beginning iterator
/// @param last End iterator
/// @param out Output iterator
/// @param pred The predicate evaluated for each element in the range
/// @param op The transform applied to each element that satisfies the predicate
/// @return The output iterator, past the last element written
template <typename Iter, typename OIter, typename Predicate,
          typename UnaryOperation>
[[nodiscard]] OIter transform_if(Iter first, Iter last, OIter out,
                                 Predicate&& pred, UnaryOperation&& op) {
  while (first != last) {
    const auto& v = *first;
    if (pred(v)) {
      *out++ = op(v);
    }
    ++first;
  }
  return out;
}

/// @brief Calls @p func once for every combination drawn from the given
/// ranges, one element from each
/// @tparam Func The visitor type
/// @tparam Iter The first range's iterator type
/// @tparam ...IterPack The remaining ranges' iterator types
/// @param func The visitor, called with one element per range
/// @param range_begin The first range's beginning
/// @param range_end The first range's end
/// @param ...other_ranges The remaining ranges, as begin/end pairs
/// @return @p func, after every call
template <typename Func, typename Iter, typename... IterPack>
Func visit_range_permutations(Func func, Iter range_begin, Iter range_end,
                              IterPack... other_ranges) {
  for (; range_begin != range_end; ++range_begin) {
    if constexpr (sizeof...(other_ranges) == 0) {
      func(*range_begin);
    } else {
      visit_range_permutations([&](auto... i) { func(*range_begin, i...); },
                               other_ranges...);
    }
  }
  return func;
}

/// @brief Calls @p f once for every ordered selection of Choose elements
/// from a range, allowing the same element to be picked more than once
/// @tparam Choose How many elements each selection holds
/// @tparam Iter The range's iterator type
/// @tparam Func The visitor type
/// @param begin The first element
/// @param end One past the last element
/// @param f The visitor, called with Choose elements
/// @return @p f, after every call
template <size_t Choose, typename Iter, typename Func>
Func visit_permutations_with_replacement(Iter begin, Iter end, Func f) {
  assert(std::distance(begin, end) >= Choose);
  if constexpr (Choose == 0) {
    return std::move(f);
  } else {
    return std::apply(
        [&](auto&... args) {
          return visit_range_permutations(std::move(f), args...);
        },
        repeat<Choose>(begin, end));
  }
}

namespace details {
template <size_t... Inds, typename Iter, typename Func>
Func permutations_without_replacement(std::index_sequence<Inds...>, Iter begin,
                                      Iter end, Func f) {
  return std::apply(
      [&](const auto&... args) {
        return visit_range_permutations(std::move(f), args...);
      },
      std::tuple_cat(std::make_tuple(std::next(begin, Inds), end)...));
}
}  // namespace details

/// @brief Calls @p f once for every ordered selection of Choose *distinct*
/// positions from a range
/// @tparam Choose How many elements each selection holds
/// @tparam Iter The range's iterator type
/// @tparam Func The visitor type
/// @param begin The first element
/// @param end One past the last element
/// @param f The visitor, called with Choose elements
/// @return @p f, after every call
template <size_t Choose, typename Iter, typename Func>
Func visit_permutations_without_replacement(Iter begin, Iter end, Func f) {
  if constexpr (Choose == 0) {
    return std::move(f);
  } else {
    return details::permutations_without_replacement(
        std::make_index_sequence<Choose>(), begin, end, std::move(f));
  }
}

/// @brief Partitions elements by a predicate applied to their transformed
/// values. The transform will only be called once per element.
/// @tparam Iter The range's iterator type
/// @tparam UnaryOperation The transform type
/// @tparam UnaryPred The predicate type
/// @param begin Beginning iterator
/// @param end End iterator
/// @param op The transform applied to each element in the range
/// @param pred The predicate evaluated on each transformed value
template <typename Iter, typename UnaryOperation, typename UnaryPred>
void partition_transform(Iter begin, Iter end, UnaryOperation&& op,
                         UnaryPred&& pred) {
  using Diff = typename std::iterator_traits<Iter>::difference_type;
  using T = typename std::iterator_traits<Iter>::value_type;
  using Key = std::remove_cv_t<
      std::remove_reference_t<std::invoke_result_t<UnaryOperation&, T&>>>;

  const Diff length = std::distance(begin, end);

  allocated_storages<Key, Diff> arrs(length, length);

  const auto& [keys_range, indices_range] = arrs;

  auto end_key_created = keys_range.begin();
  BOOST_SCOPE_EXIT_ALL(&, keys = keys_range) {
    std::destroy(keys.begin(), end_key_created);
  };

  std::for_each(begin, end,
                [&](const auto& v) { new (end_key_created++) Key{op(v)}; });

  std::iota(indices_range.begin(), indices_range.end(), Diff{});
  std::partition(indices_range.begin(), indices_range.end(),
                 [&, keys = keys_range](Diff a) {
                   return pred(keys[static_cast<size_t>(a)]);
                 });
  boost::algorithm::apply_permutation(begin, end, indices_range.begin(),
                                      indices_range.end());

  std::destroy(indices_range.begin(), indices_range.end());
}

/// @brief Reports whether two sorted ranges share a common element
/// @tparam Iter1 The first range's iterator type
/// @tparam Iter2 The second range's iterator type
/// @tparam Comp The comparison operator both ranges are sorted by
/// @param first1 The first range's beginning
/// @param last1 The first range's end
/// @param first2 The second range's beginning
/// @param last2 The second range's end
/// @param comp The comparison operator
/// @return true if any element appears in both ranges
template <class Iter1, class Iter2, class Comp = std::less<>>
bool sets_intersect(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2,
                    Comp&& comp = Comp()) {
  while (first1 != last1 && first2 != last2) {
    if (comp(*first1, *first2)) {
      ++first1;
    } else if (comp(*first2, *first1)) {
      ++first2;
    } else {
      return true;
    }
  }

  return false;
}

/// @brief Merges several sorted ranges into one sorted output range
/// @tparam Comp The comparison operator every range is sorted by
/// @tparam OutItr The output iterator type
/// @tparam ...Args The input range types
/// @param comp The comparison operator
/// @param o_it The output iterator the merged elements are written to
/// @param ...args The sorted input ranges. Each range's iterator must meet
/// the requirements of LegacyForwardIterator.
/// @return The output iterator, past the last element written
template <typename Comp, typename OutItr, typename... Args>
auto merge(Comp comp, OutItr o_it, const Args&... args) {
  using ItrType = decltype(std::begin(std::get<0>(take_first<1>(args...))));
  using ItrPairType = std::pair<ItrType, ItrType>;
  std::array<ItrPairType, sizeof...(Args)> heap{
      ItrPairType{std::begin(args), std::end(args)}...};

  auto heap_end = std::partition(heap.begin(), heap.end(), [](const auto& v) {
    return std::get<0>(v) != std::get<1>(v);
  });

  if (heap_end == heap.begin()) return o_it;

  const auto& comp_func = [&](const auto& lhs, const auto& rhs) {
    return comp(*std::get<0>(rhs), *std::get<0>(lhs));
  };

  std::make_heap(heap.begin(), heap_end, comp_func);

  while (std::next(heap.begin()) != heap_end) {
    std::pop_heap(heap.begin(), heap_end, comp_func);

    auto& [max_begin, max_end] = *std::prev(heap_end);

    do {
      *o_it++ = *max_begin++;
    } while (max_begin != max_end && comp(*max_begin, *heap[0].first));

    if (max_begin == max_end) {
      --heap_end;
    } else {
      std::push_heap(heap.begin(), heap_end, comp_func);
    }
  }

  const auto& [remain_begin, remain_end] = heap[0];
  o_it = std::copy(remain_begin, remain_end, o_it);

  return o_it;
}

/// @brief Reports whether every element of one sorted range appears in
/// another
/// @tparam Iter1 The candidate subset's iterator type
/// @tparam Iter2 The superset's iterator type
/// @tparam Comp The comparison operator both ranges are sorted by
/// @param first1 The candidate subset's beginning
/// @param last1 The candidate subset's end
/// @param first2 The superset's beginning
/// @param last2 The superset's end
/// @param comp The comparison operator
/// @return true if every element of the first range is in the second
template <class Iter1, class Iter2, class Comp = std::less<>>
bool is_subset(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2,
               Comp&& comp = Comp()) {
  while (first1 != last1 && first2 != last2) {
    if (comp(*first1, *first2)) {
      return false;
    } else if (comp(*first2, *first1)) {
      ++first2;
    } else {
      ++first1;
    }
  }

  return first1 == last1;
}

namespace details {
struct all_of {};
struct any_of {};

template <typename O, typename Q>
struct OpQuant {
  using Op = O;
  using Quantifier = Q;
};
}  // namespace details

struct eq_any_of_t : details::OpQuant<std::equal_to<void>, details::any_of> {};
inline constexpr eq_any_of_t eq_any_of;
struct eq_all_of_t : details::OpQuant<std::equal_to<void>, details::all_of> {};
inline constexpr eq_all_of_t eq_all_of;

struct ne_any_of_t
    : details::OpQuant<std::not_equal_to<void>, details::any_of> {};
inline constexpr ne_any_of_t ne_any_of;
struct ne_all_of_t
    : details::OpQuant<std::not_equal_to<void>, details::all_of> {};
inline constexpr ne_all_of_t ne_all_of;

struct gt_any_of_t : details::OpQuant<std::greater<void>, details::any_of> {};
inline constexpr gt_any_of_t gt_any_of;
struct gt_all_of_t : details::OpQuant<std::greater<void>, details::all_of> {};
inline constexpr gt_all_of_t gt_all_of;

struct ge_any_of_t
    : details::OpQuant<std::greater_equal<void>, details::any_of> {};
inline constexpr ge_any_of_t ge_any_of;
struct ge_all_of_t
    : details::OpQuant<std::greater_equal<void>, details::all_of> {};
inline constexpr ge_all_of_t ge_all_of;

struct lt_any_of_t : details::OpQuant<std::less<void>, details::any_of> {};
inline constexpr lt_any_of_t lt_any_of;
struct lt_all_of_t : details::OpQuant<std::less<void>, details::all_of> {};
inline constexpr lt_all_of_t lt_all_of;

struct le_any_of_t : details::OpQuant<std::less_equal<void>, details::any_of> {
};
inline constexpr le_any_of_t le_any_of;
struct le_all_of_t : details::OpQuant<std::less_equal<void>, details::all_of> {
};
inline constexpr le_all_of_t le_all_of;

/// @brief Compares one value against several others at once, e.g.
/// `is(x, eq_any_of, 1, 2, 3)`
/// @tparam Lhs The type of the value being compared
/// @tparam Op One of the comparison tags, such as misc::eq_any_of
/// @tparam ...Args The types of the values compared against
/// @param lhs The value being compared
/// @param args The values to compare it against
/// @return Whether the tag's comparison holds for any or all of @p args, as
/// the tag specifies
template <typename Lhs, typename Op, typename... Args>
bool is(const Lhs& lhs, Op, const Args&... args) {
  if constexpr (std::is_same_v<details::any_of, typename Op::Quantifier>)
    return (typename Op::Op{}(lhs, args) || ...);
  else if constexpr (std::is_same_v<details::all_of, typename Op::Quantifier>)
    return (typename Op::Op{}(lhs, args) && ...);
  else
    static_assert(always_false_v<Op>, ".");
}

}  // namespace misc
