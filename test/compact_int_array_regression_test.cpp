#include <gtest/gtest.h>
#include <misc/compact_int_array.h>

#include <cstdint>

// Regression test for: set_bits() copies the *middle* byte(s) of a value
// backwards when the value spans three or more bytes:
//
//   for (size_t i = start_ind + 1; i < end_ind; ++i, ptr++) {
//     *ptr = arr[i];   // should be: arr[i] = *ptr;
//   }
//
// As written, this overwrites the local `val` buffer with the array's
// *existing* bytes instead of writing val's bytes into the array, so those
// middle bytes of the array are left untouched and the value read back is
// wrong. This only shows up once a value's bit range touches 3+ bytes, so
// it needs a bit_start/bit_len combination that isn't byte-aligned.
TEST(compact_int_array_regression, SetBitsFreeFunctionMiddleByte) {
  uint8_t buf[4] = {0, 0, 0, 0};
  // bit_start = 3, bit_len = 19  =>  spans bits [3, 22), i.e. bytes 0, 1, 2.
  const size_t value = 0b101'1111'1111'1111'1111ull;  // 19 bits
  misc::set_bits(buf, value, /*bit_start=*/3, /*bit_len=*/19);
  EXPECT_EQ(misc::get_bits(buf, 3, 19), value);
}

TEST(compact_int_array_regression, SetGetMultiByteValues) {
  // A capacity of 1'000'000 needs 20 bits per element -- large enough to
  // always span 3+ bytes regardless of alignment within the array.
  using arr_type = misc::compact_int_array<1'000'000, 3>;
  ASSERT_GE(arr_type::int_bit_width, 20u);

  arr_type arr;
  const size_t v0 = 0b1010'1010'1010'1010'1010;
  const size_t v1 = 0b0101'0101'0101'0101'0101;
  const size_t v2 = 0b1111'0000'1111'0000'1111;

  arr.set(0, v0);
  arr.set(1, v1);
  arr.set(2, v2);

  EXPECT_EQ(arr.get(0), v0);
  EXPECT_EQ(arr.get(1), v1);
  EXPECT_EQ(arr.get(2), v2);
}

TEST(compact_int_array_regression, SetBitsLeavesNeighbouringBitsAlone) {
  uint8_t buf[8];
  std::fill(std::begin(buf), std::end(buf), uint8_t{0xFF});

  // Clear a 19-bit window in the middle and check nothing outside it moved.
  misc::set_bits(buf, 0, /*bit_start=*/5, /*bit_len=*/19);

  EXPECT_EQ(misc::get_bits(buf, 5, 19), 0u);
  EXPECT_EQ(misc::get_bits(buf, 0, 5), 0b11111u);
  EXPECT_EQ(misc::get_bits(buf, 24, 8), 0xFFu);
  EXPECT_EQ(buf[7], 0xFFu);
}

TEST(compact_int_array_regression, SetBitsRoundTripsAtEveryOffset) {
  for (size_t bit_len = 1; bit_len <= 32; ++bit_len) {
    for (size_t bit_start = 0; bit_start < 16; ++bit_start) {
      uint8_t buf[16] = {};
      const size_t mask =
          bit_len == 64 ? ~size_t{0} : ((size_t{1} << bit_len) - 1);
      const size_t value = 0xDEADBEEFu & mask;

      misc::set_bits(buf, value, bit_start, bit_len);
      EXPECT_EQ(misc::get_bits(buf, bit_start, bit_len), value)
          << "bit_start=" << bit_start << " bit_len=" << bit_len;
    }
  }
}

TEST(compact_int_array_regression, SetBitsMasksOffTheHighBitsOfTheValue) {
  uint8_t buf[4] = {};

  // Only the low 5 bits should land; the rest must not spill into the
  // neighbouring slot.
  misc::set_bits(buf, 0xFFFFu, /*bit_start=*/0, /*bit_len=*/5);

  EXPECT_EQ(misc::get_bits(buf, 0, 5), 0b11111u);
  EXPECT_EQ(misc::get_bits(buf, 5, 11), 0u);
}

TEST(compact_int_array_regression, GetAndSetBitRoundTrip) {
  uint8_t buf[3] = {};
  for (size_t i = 0; i < 24; ++i) {
    EXPECT_FALSE(misc::get_bit(buf, i)) << "bit " << i;
  }

  misc::set_bit(buf, true, 0);
  misc::set_bit(buf, true, 8);
  misc::set_bit(buf, true, 23);

  EXPECT_TRUE(misc::get_bit(buf, 0));
  EXPECT_TRUE(misc::get_bit(buf, 8));
  EXPECT_TRUE(misc::get_bit(buf, 23));
  EXPECT_FALSE(misc::get_bit(buf, 7));
  EXPECT_FALSE(misc::get_bit(buf, 22));

  misc::set_bit(buf, false, 8);
  EXPECT_FALSE(misc::get_bit(buf, 8));
  EXPECT_TRUE(misc::get_bit(buf, 0));
  EXPECT_TRUE(misc::get_bit(buf, 23));
}

// Regression test for: compact_int_array::operator== sized its comparison
// off `num_bits(size())` -- the number of bits needed to *represent* N --
// instead of the N * int_bit_width bits the array actually occupies. It
// also built its tail mask as ~(1 << rem) rather than (1 << rem) - 1. Both
// together meant near-arbitrary bytes were compared.
TEST(compact_int_array_regression, EqualityComparesEveryElement) {
  // 3 bits per element, 7 elements = 21 bits: two whole bytes plus a
  // 5-bit tail, so both branches of operator== are exercised.
  using arr_type = misc::compact_int_array<7, 7>;
  ASSERT_EQ(arr_type::int_bit_width, 3u);

  arr_type a;
  arr_type b;
  for (size_t i = 0; i < a.size(); ++i) {
    a.set(i, i);
    b.set(i, i);
  }

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  // Differ only in the very last element, which lives in the masked tail.
  b.set(6, 0);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  // Differ only in the first element, in the first whole byte.
  b.set(6, 6);
  ASSERT_TRUE(a == b);
  b.set(0, 5);
  EXPECT_FALSE(a == b);
}

TEST(compact_int_array_regression, EqualityIgnoresPaddingBits) {
  using arr_type = misc::compact_int_array<7, 2>;  // 6 used bits of 8
  arr_type a;
  arr_type b;
  a.set(0, 1);
  a.set(1, 2);
  b.set(0, 1);
  b.set(1, 2);
  ASSERT_TRUE(a == b);

  // Scribble on the two padding bits at the top of the only byte.
  b.storage[0] = static_cast<uint8_t>(b.storage[0] | 0b1100'0000);
  EXPECT_TRUE(a == b) << "padding bits are not part of the value";
}

TEST(compact_int_array_regression, DefaultConstructedIsAllZeros) {
  using arr_type = misc::compact_int_array<7, 5>;
  arr_type a;
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a.get(i), 0u) << "element " << i;
  }
  EXPECT_TRUE(a == arr_type{});
}

TEST(compact_int_array_regression, SingleValuedArrayStillHasStorage) {
  // MAX_INT == 0 would otherwise give a zero-bit (and zero-length) array.
  using arr_type = misc::compact_int_array<0, 4>;
  arr_type a;
  EXPECT_GE(arr_type::int_bit_width, 1u);
  EXPECT_EQ(a.get(0), 0u);
  EXPECT_EQ(a.get(3), 0u);
}

TEST(compact_int_vector_regression, PushBackGrowsAndKeepsValues) {
  misc::compact_int_vector<1000> v;  // 10 bits per element
  for (size_t i = 0; i < 100; ++i) {
    v.push_back(i * 7);
  }

  ASSERT_EQ(v.size(), 100u);
  for (size_t i = 0; i < 100; ++i) {
    EXPECT_EQ(v.get(i), i * 7) << "element " << i;
  }
}
