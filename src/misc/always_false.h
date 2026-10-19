#pragma once

#include <type_traits>

namespace misc {

/// @brief Always false, but only once T is known.
///
/// Useful for a static_assert in the unreachable branch of an
/// `if constexpr` chain, where a bare `false` would fire even when the
/// branch is discarded.
/// @tparam T Any type
template <class T>
struct always_false : std::false_type {};

/// @copydoc always_false
template <class T>
constexpr auto always_false_v = always_false<T>::value;

}  // namespace misc
