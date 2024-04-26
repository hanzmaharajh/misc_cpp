#pragma once

namespace misc {

template <class T>
struct always_false : std::false_type {};

template <class T>
constexpr auto always_false_v = always_false<T>::value;

}  // namespace misc
