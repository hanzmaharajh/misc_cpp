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
  using container_type = std::vector<T>;
  using value_compare = Comp;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;

  minmax_heap(const value_compare& comp = value_compare{}) noexcept(
      std::is_nothrow_constructible_v<minmax_heap, container_type,
                                      const value_compare&>)
      : minmax_heap(container_type{}, comp) {}

  minmax_heap(
      container_type container,
      const value_compare& comp =
          value_compare{}) noexcept(std::
                                        is_nothrow_move_constructible_v<
                                            container_type>&& std::
                                            is_nothrow_copy_constructible_v<
                                                value_compare>)
      : m_heap(std::move(container)), m_comp(comp) {
    init_sequence();
  }

  minmax_heap(const minmax_heap&) = default;
  minmax_heap(minmax_heap&&) = default;
  minmax_heap& operator=(const minmax_heap&) = default;
  minmax_heap& operator=(minmax_heap&&) = default;

  [[nodiscard]] bool empty() const { return m_heap.empty(); }

  [[nodiscard]] size_type size() const { return m_heap.size(); }

  [[nodiscard]] const_reference front() const {
    assert(!empty());
    return m_heap[0];
  }

  [[nodiscard]] const_reference back() const {
    assert(!empty());
    if (m_heap.size() < 2) return front();
    return (m_heap.size() == 2 || m_comp(m_heap[2], m_heap[1])) ? m_heap[1]
                                                                : m_heap[2];
  }

  void push(const T& t) {
    m_heap.emplace_back(t);
    push();
  }

  void push(T&& t) {
    m_heap.emplace_back(std::move(t));
    push();
  }

  void pop_front() {
    m_heap[0] = std::move(m_heap.back());
    m_heap.pop_back();
    push_down(1);
  }

  void pop_back() {
    if (m_heap.size() < 3) {
      m_heap.pop_back();
      return;
    }
    const index_type max_ind = m_comp(m_heap[1], m_heap[2]) ? 3 : 2;
    m_heap[max_ind - 1] = std::move(m_heap.back());
    m_heap.pop_back();
    push_down(max_ind);
  }

  [[nodiscard]] value_compare value_comp() const { return m_comp; }

  container_type extract_sequence() {
    return container_type{std::move(m_heap)};
  }

  void adopt_sequence(container_type&& seq) {
    m_heap = std::move(seq);
    init_sequence();
  }

  void adopt_sequence(const container_type& seq) {
    adopt_sequence(container_type{seq});
  }

  struct minmax_heap_range_t {};

  void adopt_sequence(minmax_heap_range_t, container_type&& seq) {
    m_heap = std::move(seq);
  }

  void adopt_sequence(minmax_heap_range_t, const container_type& seq) {
    adopt_sequence(container_type{seq});
  }

 protected:
  using index_type = size_type;  // 1-based
  using level_type = size_type;  // 0-based
  container_type m_heap;
  value_compare m_comp;

  auto inv_comp() const {
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

  bool is_grandchild(index_type i, index_type m) const {
    assert(i > 0);
    assert(m > i);
    return m / 4 == i;
  }

  bool has_grandparent(index_type m) const {
    assert(m > 0);
    return m > 3;
  }

  bool has_children(index_type m) const {
    assert(m > 0);
    return 2 * m <= m_heap.size();
  }

  bool is_on_min_level(index_type m) const {
    assert(m > 0);
    return (log2(m) & 0b1) == 0;
  }

  index_type parent(index_type m) const {
    assert(m > 1);
    return m / 2;
  }

  index_type grandparent(index_type m) const {
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

template <typename Iter, typename Compare = std::less<
                             typename std::iterator_traits<Iter>::value_type>>
bool is_minmax_heap(Iter begin, Iter end, const Compare& comp = Compare{}) {
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
