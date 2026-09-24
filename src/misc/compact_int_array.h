#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <memory>

#include "log2.h"

namespace misc {

/// @brief Reads a single bit out of a packed bit array
/// @param arr The array of bits, packed least significant bit first
/// @param bit_ind The index of the bit to read
/// @return The value of the bit
[[nodiscard]] inline bool get_bit(const uint8_t* arr, size_t bit_ind) {
  const auto start_ind = bit_ind / CHAR_BIT;
  const auto start_rem = bit_ind % CHAR_BIT;

  return (arr[start_ind] >> start_rem) & 0b1;
}

/// @brief Writes a single bit into a packed bit array
/// @param arr The array of bits, packed least significant bit first
/// @param val The value to write
/// @param bit_ind The index of the bit to write
inline void set_bit(uint8_t* arr, bool val, size_t bit_ind) {
  const auto start_ind = bit_ind / CHAR_BIT;
  const auto start_rem = bit_ind % CHAR_BIT;

  arr[start_ind] = static_cast<uint8_t>((~(0b1 << start_rem) & arr[start_ind]) |
                                        (val << start_rem));
}

/// @brief Reads the @p bit_len bits of @p arr starting at @p bit_start as an
/// integer, least significant bit first.
///
/// @note Works a byte at a time rather than memcpy-ing into a size_t, so it
/// neither assumes a little-endian host nor reads past the end of the bit
/// range.
/// @brief Reads a run of bits out of a packed bit array as an integer
/// @param arr The array of bits, packed least significant bit first
/// @param bit_start The index of the first bit to read
/// @param bit_len How many bits to read. At most the width of a size_t.
/// @return The bits as an integer, the bit at @p bit_start being the least
/// significant
[[nodiscard]] inline size_t get_bits(const uint8_t* arr, size_t bit_start,
                                     size_t bit_len) {
  assert(bit_len <= sizeof(size_t) * CHAR_BIT);

  size_t retval{};
  for (size_t read = 0; read < bit_len;) {
    const auto byte_ind = (bit_start + read) / CHAR_BIT;
    const auto bit_off = (bit_start + read) % CHAR_BIT;
    const auto num = std::min<size_t>(CHAR_BIT - bit_off, bit_len - read);

    const size_t chunk = (static_cast<size_t>(arr[byte_ind]) >> bit_off) &
                         ((size_t{1} << num) - size_t{1});
    retval |= chunk << read;
    read += num;
  }

  return retval;
}

/// @brief Writes the low @p bit_len bits of @p val into @p arr starting at
/// @p bit_start, least significant bit first. Bits outside that range are
/// left untouched.
/// @brief Writes an integer into a run of bits of a packed bit array
/// @param arr The array of bits, packed least significant bit first
/// @param val The value to write. Bits above @p bit_len are ignored.
/// @param bit_start The index of the first bit to write
/// @param bit_len How many bits to write. At most the width of a size_t.
/// @note Bits outside [bit_start, bit_start + bit_len) are left untouched.
inline void set_bits(uint8_t* arr, size_t val, size_t bit_start,
                     size_t bit_len) {
  assert(bit_len <= sizeof(size_t) * CHAR_BIT);

  for (size_t written = 0; written < bit_len;) {
    const auto byte_ind = (bit_start + written) / CHAR_BIT;
    const auto bit_off = (bit_start + written) % CHAR_BIT;
    const auto num = std::min<size_t>(CHAR_BIT - bit_off, bit_len - written);

    const auto mask =
        static_cast<uint8_t>(((size_t{1} << num) - size_t{1}) << bit_off);
    const auto bits =
        static_cast<uint8_t>(((val >> written) << bit_off) & mask);

    arr[byte_ind] = static_cast<uint8_t>((arr[byte_ind] & ~mask) | bits);
    written += num;
  }
}

/// @brief A fixed-size array of N unsigned integers, each stored in only as
/// many bits as MAX_INT needs.
/// @tparam MAX_INT The largest value any element may hold
/// @tparam N The number of elements
template <size_t MAX_INT, size_t N>
class compact_int_array {
  // TODO Use log2() when it is constexpr
  [[nodiscard]] static constexpr size_t num_bits(size_t v) {
    return v == 0 ? 0 : (v == 1 ? 1 : 1 + num_bits(v >> 1));
  }

 public:
  // At least one bit, so that MAX_INT == 0 doesn't produce a zero-length
  // storage array.
  /// @brief How many bits each element occupies. At least one, so that
  /// MAX_INT == 0 doesn't produce a zero-length allocation.
  static const constexpr size_t int_bit_width =
      num_bits(MAX_INT) == 0 ? 1 : num_bits(MAX_INT);
  /// @brief The size of the backing byte array
  static const constexpr size_t arr_length =
      (N * int_bit_width + CHAR_BIT - 1) / CHAR_BIT;

  /// @brief How many bits of the backing array hold elements. The remainder
  /// of the last byte is padding.
  static const constexpr size_t used_bits = N * int_bit_width;

  /// @brief The packed elements. Zero-initialised.
  uint8_t storage[arr_length]{};

  /// @brief Returns the element at @p ind
  /// @param ind The element index. It must be less than N.
  /// @return The element's value
  [[nodiscard]] size_t get(size_t ind) const {
    assert(ind < N);
    return get_bits(storage, ind * int_bit_width, int_bit_width);
  }

  /// @brief Assigns the element at @p ind
  /// @param ind The element index. It must be less than N.
  /// @param val The value to store. It must not exceed MAX_INT.
  void set(size_t ind, size_t val) {
    assert(val <= MAX_INT);
    assert(ind < N);
    set_bits(storage, val, ind * int_bit_width, int_bit_width);
  }

  /// @brief Returns the number of elements
  /// @return N
  [[nodiscard]] constexpr size_t size() const { return N; }

  /// @brief Compares every element
  /// @param o The array to compare against
  /// @return true if every element is equal
  /// @note Only the used_bits bits in use are compared; the padding bits at
  /// the end of the last byte are not part of the value.
  [[nodiscard]] bool operator==(const compact_int_array& o) const {
    constexpr size_t whole_bytes = used_bits / CHAR_BIT;
    constexpr size_t rem_bits = used_bits % CHAR_BIT;

    if (!std::equal(storage, storage + whole_bytes, o.storage)) return false;

    if constexpr (rem_bits == 0) {
      return true;
    } else {
      const auto mask = static_cast<uint8_t>((1u << rem_bits) - 1u);
      return (storage[whole_bytes] & mask) == (o.storage[whole_bytes] & mask);
    }
  }

  /// @copydoc operator==()
  [[nodiscard]] bool operator!=(const compact_int_array& o) const {
    return !(*this == o);
  }
};

/// @brief A growable sequence of unsigned integers, each stored in only as
/// many bits as MAX_INT needs.
/// @tparam MAX_INT The largest value any element may hold
template <size_t MAX_INT>
class compact_int_vector {
  // TODO Use log2() when it is constexpr
  [[nodiscard]] static constexpr size_t num_bits(size_t v) {
    return v == 0 ? 0 : (v == 1 ? 1 : 1 + num_bits(v >> 1));
  }

 public:
  // At least one bit, so that MAX_INT == 0 doesn't produce zero-length
  // allocations.
  /// @brief How many bits each element occupies. At least one, so that
  /// MAX_INT == 0 doesn't produce a zero-length allocation.
  static const constexpr size_t int_bit_width =
      num_bits(MAX_INT) == 0 ? 1 : num_bits(MAX_INT);
  size_t m_size = 0;
  size_t m_capacity = 0;

  /// @brief Returns how many bytes @p num_ints packed elements occupy
  /// @param num_ints The number of elements
  /// @return The required byte count
  static constexpr size_t arr_length(size_t num_ints) {
    return (num_ints * int_bit_width + CHAR_BIT - 1) / CHAR_BIT;
  }

  /// @brief The packed elements
  std::unique_ptr<uint8_t[]> storage;

  /// @brief Returns the number of elements
  /// @return The current size
  [[nodiscard]] size_t size() const { return m_size; }

  /// @brief Returns how many elements fit without reallocating
  /// @return The current capacity
  [[nodiscard]] size_t capacity() const { return m_capacity; }

  /// @brief Returns the element at @p ind
  /// @param ind The element index. It must be less than size().
  /// @return The element's value
  [[nodiscard]] size_t get(size_t ind) const {
    assert(ind < m_size);
    return get_bits(storage.get(), ind * int_bit_width, int_bit_width);
  }

  /// @brief Assigns the element at @p ind
  /// @param ind The element index. It must be less than capacity().
  /// @param val The value to store. It must not exceed MAX_INT.
  void set(size_t ind, size_t val) {
    assert(val <= MAX_INT);
    assert(ind < m_capacity);
    set_bits(storage.get(), val, ind * int_bit_width, int_bit_width);
  }

  /// @brief Appends an element, growing the allocation if needed
  /// @param val The value to append. It must not exceed MAX_INT.
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