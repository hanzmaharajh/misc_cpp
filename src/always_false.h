#pragma once

namespace misc {

// Taken from https://en.cppreference.com/w/cpp/utility/variant/visit
template <class T>
struct always_false : std::false_type {};

}  // namespace misc
