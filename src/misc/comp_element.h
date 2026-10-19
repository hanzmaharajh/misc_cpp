#pragma once

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>

namespace misc {

/// @brief A comparator that orders tuple-like values by a chosen subset of
/// their elements, in the order the indices are given.
/// @tparam Comp The comparator applied to the extracted elements
/// @tparam ...Ind The element indices to compare by
template <typename Comp = std::less<>, size_t... Ind>
class compare_by_elements {
  Comp m_comp;

 public:
  /// @brief Constructs a comparator
  /// @param comp The comparator applied to the extracted elements
  compare_by_elements(Comp comp = Comp{}) : m_comp(std::move(comp)) {}

  /// @brief Compares two tuple-like values by their Ind-th elements
  /// @tparam L The left-hand type
  /// @tparam R The right-hand type
  /// @param l The left-hand value
  /// @param r The right-hand value
  /// @return Whatever Comp reports for the extracted elements
  template <typename L, typename R>
  [[nodiscard]] bool operator()(const L& l, const R& r) const {
    return m_comp(std::forward_as_tuple(std::get<Ind>(l)...),
                  std::forward_as_tuple(std::get<Ind>(r)...));
  }
};

/// @brief compare_by_elements for a single element
/// @tparam Ind The element index to compare by
/// @tparam Comp The comparator applied to the extracted element
template <size_t Ind, typename Comp = std::less<>>
using compare_by_element = compare_by_elements<Comp, Ind>;

}  // namespace misc