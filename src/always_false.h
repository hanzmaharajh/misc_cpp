#pragma once

namespace misc {

// Taken from https://en.cppreference.com/w/cpp/utility/variant/visit
template <class T>
struct always_false : std::false_type {};

template <class T>
constexpr auto always_false_v = always_false<T>::value;

}  // namespace misc
