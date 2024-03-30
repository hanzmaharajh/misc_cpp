#pragma once

namespace misc {
#if __cplusplus >= 202002L
#include <bit>
#elif defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#include <climits>
#if defined(_MSC_VER)
#include <immintrin.h>
#endif
#endif

template <typename T>
[[nodiscard]] static inline int log2(T i) {
  assert(i > 0);
#if __cplusplus >= 202002L
  return std::bit_width(i) - 1;
#else
#if defined(_MSC_VER)
  static_assert(std::is_same_v<T, uint64_t>);
  return static_cast<int>(sizeof(i)) * CHAR_BIT - 1 - _lzcnt_u64(i);
#elif defined(__GNUC__) || defined(__clang__)
  if constexpr (std::is_same_v<T, unsigned long long>) {
    return static_cast<int>(sizeof(i)) * CHAR_BIT - 1 - __builtin_clzll(i);
  } else if constexpr (std::is_same_v<T, unsigned long>) {
    return static_cast<int>(sizeof(i)) * CHAR_BIT - 1 - __builtin_clzl(i);
  } else if constexpr (std::is_same_v<T, unsigned int>) {
    return static_cast<int>(sizeof(i)) * CHAR_BIT - 1 - __builtin_clz(i);
  }
#else
  int retval = 0;
  while (i >>= 1) ++retval;
  return retval;
#endif
#endif
}
}  // namespace misc
