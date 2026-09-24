#pragma once
#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include "log2.h"

namespace misc {
/// @brief A min-max heap
/// @tparam T The element type
/// @tparam Comp The comparison operator
/// @note This code is based on the paper, "Min-Max Heaps and Generalized
/// Priority Queues" by M.D.ATKINSON, J.- R.SACK, N.SANTORO, and T.STROTHOTTE,
/// and the wikipedia article at https://en.wikipedia.org/wiki/Min-max_heap
template <typename T, typename Comp = std::less<T>>
class minmax_heap {
 public:
  /// @brief The underlying sequence the heap is laid out in
  using container_type = std::vector<T>;

  /// @brief The comparison operator ordering the elements
  using value_compare = Comp;

  /// @brief The element type
  using value_type = typename container_type::value_type;

  /// @brief The type used for sizes and indices
  using size_type = typename container_type::size_type;

  /// @brief A reference to an element
  using reference = typename container_type::reference;

  /// @brief A const reference to an element
  using const_reference = typename container_type::const_reference;

  /// @brief Constructs an empty heap
  /// @param comp The comparison operator
  minmax_heap(const value_compare& comp = value_compare{}) noexcept(
      std::is_nothrow_constructible_v<minmax_heap, container_type,
                                      const value_compare&>)
      : minmax_heap(container_type{}, comp) {}

  /// @brief Constructs a heap from an existing sequence, reordering it
  /// into min-max heap order
  /// @param container The elements to take over
  /// @param comp The comparison operator
  minmax_heap(
      container_type container,
      const value_compare& comp =
          value_compare{}) noexcept(std::
                                        is_nothrow_move_constructible_v<
                                            container_type> &&
                                    std::is_nothrow_copy_constructible_v<
                                        value_compare>)
      : m_heap(std::move(container)), m_comp(comp) {
    init_sequence();
  }

  /// @brief Copies another heap
  minmax_heap(const minmax_heap&) = default;

  /// @brief Takes over another heap's elements
  minmax_heap(minmax_heap&&) = default;

  /// @copydoc minmax_heap(const minmax_heap&)
  minmax_heap& operator=(const minmax_heap&) = default;

  /// @copydoc minmax_heap(minmax_heap&&)
  minmax_heap& operator=(minmax_heap&&) = default;

  /// @brief Exchanges contents with @p o
  /// @param o The heap to exchange with
  void swap(minmax_heap& o) noexcept {
    using std::swap;
    swap(m_heap, o.m_heap);
    swap(m_comp, o.m_comp);
  }

  /// @brief Reports whether the heap holds no elements
  /// @return true if the heap is empty
  [[nodiscard]] bool empty() const { return m_heap.empty(); }

  /// @brief Returns the number of elements
  /// @return The element count
  [[nodiscard]] size_type size() const { return m_heap.size(); }

  /// @brief Grows the underlying sequence to hold at least @p n elements
  /// @param n The capacity to make room for
  void reserve(size_t n) { m_heap.reserve(n); }

  /// @brief Returns the smallest element
  /// @return A reference to the minimum. The heap must not be empty.
  [[nodiscard]] const_reference front() const {
    assert(!empty());
    return m_heap.front();
  }

  /// @brief Returns the largest element
  /// @return A reference to the maximum. The heap must not be empty.
  [[nodiscard]] const_reference back() const {
    assert(!empty());
    if (m_heap.size() < 2) return front();
    return (m_heap.size() == 2 || m_comp(m_heap[2], m_heap[1])) ? m_heap[1]
                                                                : m_heap[2];
  }

  /// @brief Inserts a copy of @p t
  /// @param t The element to insert
  void push(const T& t) {
    m_heap.emplace_back(t);
    push();
  }

  /// @brief Inserts @p t
  /// @param t The element to move in
  void push(T&& t) {
    m_heap.emplace_back(std::move(t));
    push();
  }

  /// @brief Inserts an element constructed in place
  /// @tparam ...Args The constructor argument types
  /// @param ...args The arguments forwarded to T's constructor
  template <typename... Args>
  void emplace(Args&&... args) {
    m_heap.emplace_back(std::forward<Args>(args)...);
    push();
  }

  /// @brief Removes the smallest element
  /// @note The heap must not be empty.
  void pop_front() {
    assert(!m_heap.empty());
    m_heap.front() = std::move(m_heap.back());
    m_heap.pop_back();
    push_down(1);
  }

  /// @brief Removes the largest element
  /// @note The heap must not be empty.
  void pop_back() {
    assert(!m_heap.empty());
    if (m_heap.size() < 3) {
      m_heap.pop_back();
      return;
    }
    const index_type max_ind = m_comp(m_heap[1], m_heap[2]) ? 3 : 2;
    m_heap[max_ind - 1] = std::move(m_heap.back());
    m_heap.pop_back();
    push_down(max_ind);
  }

  /// @brief Returns the comparison operator
  /// @return A reference to the comparator
  [[nodiscard]] const value_compare& value_comp() const { return m_comp; }

  /// @brief Hands the underlying sequence over to the caller
  /// @return The elements, still in min-max heap order. The heap is left in
  /// a valid but unspecified state.
  container_type extract_sequence() {
    return container_type{std::move(m_heap)};
  }

  /// @brief Replaces the contents with @p seq, reordering it into min-max
  /// heap order
  /// @param seq The elements to take over
  void adopt_sequence(container_type&& seq) {
    m_heap = std::move(seq);
    init_sequence();
  }

  /// @copydoc adopt_sequence(container_type&&)
  void adopt_sequence(const container_type& seq) {
    adopt_sequence(container_type{seq});
  }

  /// @brief Tag asserting that a sequence is already in min-max heap order
  struct minmax_heap_range_t {};

  /// @brief Replaces the contents with @p seq without reordering it
  /// @param seq The elements to take over. They must already be in min-max
  /// heap order; see is_minmax_heap().
  void adopt_sequence(minmax_heap_range_t, container_type&& seq) {
    m_heap = std::move(seq);
  }

  /// @copydoc adopt_sequence(minmax_heap_range_t, container_type&&)
  void adopt_sequence(minmax_heap_range_t, const container_type& seq) {
    adopt_sequence(container_type{seq});
  }

 protected:
  using index_type = size_type;  // 1-based
  using level_type = size_type;  // 0-based
  container_type m_heap;
  value_compare m_comp;

  [[nodiscard]] auto inv_comp() const {
    return [&](const auto& l, const auto& r) { return m_comp(r, l); };
  }

  void init_sequence() {
    if (m_heap.size() < 2) {
      return;
    }
    for (index_type i = m_heap.size() / 2; i > 0; --i) {
      push_down(i);
    }
  }

  void push_down(index_type m) {
    assert(m > 0);

    while (has_children(m)) {
      index_type i = m;
      if (is_on_min_level(i)) {
        m = push_down(i, m_comp);
      } else {
        m = push_down(i, inv_comp());
      }
    }
  }

  template <typename CompFunc>
  index_type push_down(index_type i, const CompFunc& comp) {
    using std::swap;
    const auto m = min_child_or_grandchild(i, comp);
    if (comp(m_heap[m - 1], m_heap[i - 1])) {
      swap(m_heap[m - 1], m_heap[i - 1]);
      if (is_grandchild(i, m)) {
        const auto p = parent(m);
        if (comp(m_heap[p - 1], m_heap[m - 1])) {
          swap(m_heap[p - 1], m_heap[m - 1]);
        }
        return m;
      }
    }
    return m_heap.size();  // return an index we know has no children
  }

  template <typename CompFunc>
  index_type min_child_or_grandchild(index_type m, const CompFunc& comp) const {
    assert(has_children(m));
    const index_type left_child_ind = 2 * m;
    const index_type right_child_ind = 2 * m + 1;
    const index_type left_left_grandchild = 2 * left_child_ind;
    const index_type right_left_grandchild = 2 * left_child_ind + 1;
    const index_type left_right_grandchild = 2 * right_child_ind;
    const index_type right_right_grandchild = 2 * right_child_ind + 1;

    const auto* min_val = &m_heap[left_child_ind - 1];
    index_type retval = left_child_ind;

    for (index_type i :
         {right_child_ind, left_left_grandchild, right_left_grandchild,
          left_right_grandchild, right_right_grandchild}) {
      if (m_heap.size() < i) {
        return retval;
      }
      if (comp(m_heap[i - 1], *min_val)) {
        min_val = &m_heap[i - 1];
        retval = i;
      }
    }
    return retval;
  }

  [[nodiscard]] constexpr static bool is_grandchild(index_type i,
                                                    index_type m) {
    assert(i > 0);
    assert(m > i);
    return m / 4 == i;
  }

  [[nodiscard]] constexpr static bool has_grandparent(index_type m) {
    assert(m > 0);
    return m > 3;
  }

  [[nodiscard]] bool has_children(index_type m) const {
    assert(m > 0);
    return 2 * m <= m_heap.size();
  }

  [[nodiscard]] constexpr static bool is_on_min_level(index_type m) {
    assert(m > 0);
    return (log2(m) & 0b1) == 0;
  }

  [[nodiscard]] constexpr static index_type parent(index_type m) {
    assert(m > 1);
    return m / 2;
  }

  [[nodiscard]] constexpr static index_type grandparent(index_type m) {
    assert(m > 2);
    return m / 4;
  }

  void push() {
    const auto i = m_heap.size();
    if (i == 1) return;

    if (is_on_min_level(i)) {
      push(i, m_comp, inv_comp());
    } else {
      push(i, inv_comp(), m_comp);
    }
  }

  template <typename CompFunc, typename InvCompFunc>
  void push(index_type i, const CompFunc& comp, const InvCompFunc& inv_comp) {
    using std::swap;
    const auto p = parent(i);
    if (comp(m_heap[p - 1], m_heap[i - 1])) {
      swap(m_heap[p - 1], m_heap[i - 1]);
      push(p, inv_comp);
    } else {
      push(i, comp);
    }
  }

  template <typename CompFunc>
  void push(index_type i, const CompFunc& comp) {
    using std::swap;
    while (true) {
      if (!has_grandparent(i)) return;
      const auto gp = grandparent(i);
      if (!comp(m_heap[i - 1], m_heap[gp - 1])) return;
      swap(m_heap[i - 1], m_heap[gp - 1]);
      i = gp;
    }
  }
};

/// @brief Reports whether a range is laid out as a min-max heap
/// @tparam Iter The range's iterator type
/// @tparam Compare The comparison operator
/// @param begin The first element
/// @param end One past the last element
/// @param comp The comparison operator
/// @return true if the range satisfies the min-max heap invariant
template <typename Iter, typename Compare = std::less<
                             typename std::iterator_traits<Iter>::value_type>>
[[nodiscard]] bool is_minmax_heap(Iter begin, Iter end,
                                  const Compare& comp = Compare{}) {
  const auto length = std::distance(begin, end);
  if (length < 2) {
    return true;
  }

  const auto& is_on_min_level = [&](size_t m) {
    assert(m > 0);
    return (log2(m) & 0b1) == 0;
  };

  using diff_type = typename std::iterator_traits<Iter>::difference_type;

  size_t i = 2;
  for (auto it = std::next(begin); it != end; ++it, ++i) {
    if (is_on_min_level(i)) {
      if (comp(*std::next(begin, static_cast<diff_type>(i / 2 - 1)), *it)) {
        return false;
      }
    } else {
      if (comp(*it, *std::next(begin, static_cast<diff_type>(i / 2 - 1)))) {
        return false;
      }
    }
  }

  return true;
}
}  // namespace misc
