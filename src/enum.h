#pragma once

#include <type_traits>

namespace misc {
template <typename Enum>
auto underlying_value(Enum e) {
  return static_cast<std::underlying_type_t<Enum>>(e);
}
}  // namespace misc