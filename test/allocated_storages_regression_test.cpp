#include <gtest/gtest.h>
#include <misc/allocated_storages.h>

#include <cstddef>
#include <cstdint>
#include <exception>

namespace {
// Same shape as the Thrower helper in allocated_storages_test.cpp, but kept
// local to this file so it doesn't need that file's internals.
template <size_t ID, size_t OnConstruction, size_t OnCopy>
struct RegressionThrower {
  static size_t constructed, copied, destroyed;
  RegressionThrower() {
    if (OnConstruction != 0 && constructed + 1 == OnConstruction) {
      throw std::exception();
    }
    ++constructed;
  }
  RegressionThrower(const RegressionThrower&) {
    if (OnCopy != 0 && copied + 1 == OnCopy) {
      throw std::exception();
    }
    ++copied;
  }
  ~RegressionThrower() { ++destroyed; }
  static void reset() { constructed = copied = destroyed = 0; }
};
template <size_t ID, size_t OnConstruction, size_t OnCopy>
size_t RegressionThrower<ID, OnConstruction, OnCopy>::constructed = 0;
template <size_t ID, size_t OnConstruction, size_t OnCopy>
size_t RegressionThrower<ID, OnConstruction, OnCopy>::copied = 0;
template <size_t ID, size_t OnConstruction, size_t OnCopy>
size_t RegressionThrower<ID, OnConstruction, OnCopy>::destroyed = 0;
}  // namespace

// Regression test for: when construction of the N'th array (via the
// std::initializer_list constructor) throws, init_list's cleanup path calls
//   del_arrs(std::make_index_sequence<Ind - 1>())
// which only destroys arrays 0..Ind-2, silently skipping array Ind-1 -- the
// one built *immediately* before the one that failed. That array's elements
// are leaked (their destructors never run), even though the failing array
// and everything before Ind-1 are cleaned up correctly.
//
// This needs at least 3 arrays, with the throwing one in the last position
// and a *tracked* (non-trivial) type immediately before it, or the leak is
// invisible (a leaked `char` has no observable destructor side effect,
// which is why the existing exception_on_init_list test -- which only uses
// two arrays -- doesn't catch this).
TEST(allocated_storages_regression,
     InitListExceptionDestroysImmediatelyPrecedingArray) {
  using Preceding = RegressionThrower<__COUNTER__, 0, 0>;  // never throws
  using Failing = RegressionThrower<__COUNTER__, 0, 3>;    // throws on 3rd copy
  Preceding::reset();
  Failing::reset();

  bool threw = false;
  try {
    const misc::unique_arrays arr({'a', 'b'}, {Preceding{}, Preceding{}},
                                  {Failing{}, Failing{}, Failing{}, Failing{}});
  } catch (const std::exception&) {
    threw = true;
  }
  ASSERT_TRUE(threw);

  // Two default-constructed temporaries for the initializer_list, each then
  // copied into the array -- so four live objects, all of which must be
  // destroyed (the temporaries at the end of the full expression, the array
  // copies by the cleanup path).
  EXPECT_EQ(Preceding::constructed, 2u);
  EXPECT_EQ(Preceding::copied, 2u);
  EXPECT_EQ(Preceding::destroyed, Preceding::constructed + Preceding::copied)
      << "the array built right before the failing one was leaked";
}

// Same bug, but confirms the already-fixed init_arr() path (the
// std::initializer_list<size_t> "count" constructor) keeps behaving
// correctly with 3+ arrays, not just the 2-array case already covered.
TEST(allocated_storages_regression,
     InitCountExceptionDestroysImmediatelyPrecedingArray) {
  using Preceding = RegressionThrower<__COUNTER__, 0, 0>;  // never throws
  using Failing = RegressionThrower<__COUNTER__, 3, 0>;    // throws on 3rd ctor
  Preceding::reset();
  Failing::reset();

  bool threw = false;
  try {
    const misc::unique_arrays<char, Preceding, Failing> arr(2, 2, 5);
  } catch (const std::exception&) {
    threw = true;
  }
  ASSERT_TRUE(threw);

  EXPECT_EQ(Preceding::constructed, 2u);
  EXPECT_EQ(Preceding::destroyed, Preceding::constructed);
}

// Regression test for: allocate_storage() used to size the whole block's
// alignment off of just the *first* Args type (alignof(nth_type<0>[])),
// instead of the maximum alignment across all Args. Put a small, weakly
// aligned type first and a strongly aligned type after it, and check the
// second array's address is actually aligned -- not just aligned relative
// to the (possibly-misaligned) start of the block.
TEST(allocated_storages_regression, BlockIsAlignedForLargestType) {
  misc::unique_arrays<char, double> arr(1, 8);
  const auto double_range = arr.get<1>();
  const auto addr = reinterpret_cast<uintptr_t>(double_range.begin());
  EXPECT_EQ(addr % alignof(double), 0u);
}
