#pragma once

namespace misc {

/// @brief Combines several callables into one overload set, for use as a
/// visitor.
///
/// Taken from https://en.cppreference.com/w/cpp/utility/variant/visit
/// @tparam ...Ts The callable types to combine
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
/// @brief Deduces the callable types from the objects passed in. Not
/// needed as of C++20.
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace misc
