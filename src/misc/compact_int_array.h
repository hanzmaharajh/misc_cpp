#pragma once

#include <algorithm>
#include <cassert>
#include <memory>

#include "log2.h"

namespace misc {

[[nodiscard]] inline bool get_bit(const uint8_t* arr, size_t bit_ind) {
  const auto start_ind = bit_ind / CHAR_BIT;
  const auto start_rem = bit_ind % CHAR_BIT;

  return (arr[start_ind] >> start_rem) & 0b1;
}

inline void set_bit(uint8_t* arr, bool val, size_t bit_ind) {
  const auto start_ind = bit_ind / CHAR_BIT;
  const auto start_rem = bit_ind % CHAR_BIT;

  arr[start_ind] = static_cast<uint8_t>((~(0b1 << start_rem) & arr[start_ind]) |
                                        (val << start_rem));
}

[[nodiscard]] inline size_t get_bits(const uint8_t* arr, size_t bit_start,
                                     size_t bit_len) {
  assert(bit_len <= sizeof(size_t) * CHAR_BIT);
  const auto start_ind = bit_start / CHAR_BIT;
  const auto start_rem = bit_start % CHAR_BIT;
  const auto end_ind = (bit_start + bit_len) / CHAR_BIT;
  const auto end_rem = (bit_start + bit_len) % CHAR_BIT;

  size_t retval{};
  uint8_t* ptr = reinterpret_cast<uint8_t*>(&retval);

  std::copy_n(arr + start_ind, end_ind - start_ind + (end_rem > 0), ptr);

  retval >>= start_rem;

  if (end_rem) {
    const auto mask =
        (size_t{1} << (end_rem + (end_ind - start_ind) * CHAR_BIT -
                       start_rem)) -
        size_t{1};
    retval &= mask;
  }

  return retval;
}

inline void set_bits(uint8_t* arr, size_t val, size_t bit_start,
                     size_t bit_len) {
  assert(bit_len <= sizeof(size_t) * CHAR_BIT);
  const auto start_ind = bit_start / CHAR_BIT;
  const auto start_rem = bit_start % CHAR_BIT;
  const auto end_ind = (bit_start + bit_len) / CHAR_BIT;
  const auto end_rem = (bit_start + bit_len) % CHAR_BIT;

  uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);

  const auto mask = ((size_t{1} << bit_len) - size_t{1}) << start_rem;
  arr[start_ind] &= static_cast<uint8_t>(~mask);
  arr[start_ind] |= static_cast<uint8_t>(*ptr << start_rem);
  val >>= CHAR_BIT - start_rem;

  for (size_t i = start_ind + 1; i < end_ind; ++i, ptr++) {
    *ptr = arr[i];
  }

  if (end_rem && end_ind != start_ind) {
    auto m = static_cast<uint8_t>(~((size_t{1} << end_rem) - size_t{1}));
    arr[end_ind] &= m;
    auto v =
        static_cast<uint8_t>((*ptr & ((size_t{1} << end_rem) - size_t{1})));
    arr[end_ind] |= v;
  }
}

template <size_t MAX_INT, size_t N>
class compact_int_array {
  // TODO Use log2() when it is constexpr
  [[nodiscard]] static constexpr size_t num_bits(size_t v) {
    return v == 0 ? 0 : (v == 1 ? 1 : 1 + num_bits(v >> 1));
  }

 public:
  static const constexpr size_t int_bit_width = num_bits(MAX_INT);
  static const constexpr size_t arr_length =
      (N * int_bit_width + CHAR_BIT - 1) / CHAR_BIT;
  uint8_t storage[arr_length];

  [[nodiscard]] size_t get(size_t ind) const {
    assert(ind < N);
    return get_bits(storage, ind * int_bit_width, int_bit_width);
  }

  void set(size_t ind, size_t val) {
    assert(val <= MAX_INT);
    assert(ind < N);
    set_bits(storage, val, ind * int_bit_width, int_bit_width);
  }
};

template <size_t MAX_INT>
class compact_int_vector {
  // TODO Use log2() when it is constexpr
  [[nodiscard]] static constexpr size_t num_bits(size_t v) {
    return v == 0 ? 0 : (v == 1 ? 1 : 1 + num_bits(v >> 1));
  }

 public:
  static const constexpr size_t int_bit_width = num_bits(MAX_INT);
  size_t m_size = 0;
  size_t m_capacity = 0;
  static constexpr size_t arr_length(size_t num_ints) {
    return (num_ints * int_bit_width + CHAR_BIT - 1) / CHAR_BIT;
  }
  std::unique_ptr<uint8_t[]> storage;

  [[nodiscard]] size_t size() const { return m_size; }

  [[nodiscard]] size_t capacity() const { return m_capacity; }

  [[nodiscard]] size_t get(size_t ind) const {
    return get_bits(storage.get(), ind * int_bit_width, int_bit_width);
  }

  void set(size_t ind, size_t val) {
    assert(val <= MAX_INT);
    set_bits(storage.get(), val, ind * int_bit_width, int_bit_width);
  }

  void push_back(size_t val) {
    if (size() >= capacity()) {
      auto new_capacity = std::max<size_t>(2 * size(), 1);
      const auto& len = arr_length(new_capacity);
      new_capacity = len * CHAR_BIT / int_bit_width;
      auto p = std::make_unique<uint8_t[]>(len);
      if (storage) {
        std::copy_n(storage.get(), arr_length(capacity()), p.get());
      }
      m_capacity = new_capacity;
      storage = std::move(p);
    }
    set(size(), val);
    ++m_size;
  }
};

}  // namespace misc