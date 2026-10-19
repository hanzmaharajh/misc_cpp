#pragma once

#include <type_traits>

namespace misc {
/// @brief Converts an enumerator to its underlying integral value
/// @tparam Enum The enumeration type
/// @param e The enumerator
/// @return @p e as its underlying type
template <typename Enum>
[[nodiscard]] constexpr auto underlying_value(Enum e) {
  return static_cast<std::underlying_type_t<Enum>>(e);
}
}  // namespace misc