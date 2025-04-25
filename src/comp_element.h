#pragma once

#include <functional>

namespace misc {

template <typename Comp = std::less<>, size_t... Ind>
class compare_by_elements {
  Comp m_comp;

 public:
  compare_by_elements(Comp comp = Comp{}) : m_comp(std::move(comp)) {}

  template <typename L, typename R>
  [[nodiscard]] bool operator()(const L& l, const R& r) const {
    return m_comp(std::forward_as_tuple(std::get<Ind>(l)...),
                  std::forward_as_tuple(std::get<Ind>(r)...));
  }
};

template <size_t Ind, typename Comp = std::less<>>
using compare_by_element = compare_by_elements<Comp, Ind>;

}  // namespace misc