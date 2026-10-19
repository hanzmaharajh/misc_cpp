#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/vector_of_optional.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

// NOTE: these tests use the nullable-handle accessors (`operator[]` and
// `*iterator` return a pointer that is null for a disengaged slot). gtest's
// ASSERT_NE(p, nullptr) is not a null check GCC can follow through at -O3,
// so every subsequent dereference trips -Wnull-dereference. Same suppression
// (and same TODO) as dense_index_map_test.cpp.
// TODO Sort this out. There's an issue with the iterators.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

// Regression test for: operator-(const_iterator, const_iterator) does
// `return lhs.in - rhs.ind;` -- `in` isn't a member (it's `ind`), so this
// shouldn't compile.
TEST(vector_of_optional_regression, IteratorDifference) {
  misc::vector_of_optional<int> v;
  v.emplace_back(1);
  v.emplace_back(2);
  v.emplace_back(3);

  EXPECT_EQ(v.end() - v.begin(), 3);
  EXPECT_EQ(v.begin() - v.begin(), 0);
  EXPECT_EQ((v.begin() + 2) - v.begin(), 2);
}

// Regression test for: clear() calls clear(i) in its loop, but the class
// only defines reset(size_t), not clear(size_t) -- shouldn't compile as
// written.
TEST(vector_of_optional_regression, ClearEmptiesEveryElement) {
  misc::vector_of_optional<int> v;
  v.emplace_back(1);
  v.emplace_back(2);
  ASSERT_EQ(v.size(), 2u);

  v.clear();

  EXPECT_EQ(v.size(), 0u);
}

// Regression test for: operator== indexes into the raw underlying storage
// range (arr_range<T>::operator[], a T&) instead of this class's own
// operator[] (a T*) to check whether a slot is set:
//   if (const auto& l_ptr = l_data_range[i]) { ... }
// For a T with no operator bool() this doesn't compile; for something like
// `int` it compiles but silently treats a *present* value of 0 as "unset"
// and skips comparing it, which can make unequal vectors compare equal.
TEST(vector_of_optional_regression, EqualityDoesNotTreatZeroAsUnset) {
  misc::vector_of_optional<int> a;
  a.emplace_back(0);  // a real, present value that happens to be falsy
  a.resize(2);        // second slot left unset in both

  misc::vector_of_optional<int> b;
  b.emplace_back(5);
  b.resize(2);

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  misc::vector_of_optional<int> c;
  c.emplace_back(0);
  c.resize(2);

  EXPECT_TRUE(a == c);
}

// Regression test for: the protected is_bit_set() helper computed its bit
// mask as `1 << (i & ((1 << CHAR_BIT) - 1))`, i.e. `i & 255` where it meant
// `i % CHAR_BIT`. For any index of 8 or more the shift ran off the end of
// the byte and the helper always answered "not set" -- so on a reallocation
// (and on destruction) every element from index 8 onwards was silently
// skipped: not copied across, not destroyed.
TEST(vector_of_optional_regression, GrowingPastTheFirstBitmapByte) {
  constexpr size_t count = 40;

  misc::vector_of_optional<std::shared_ptr<int>> v;
  std::vector<std::shared_ptr<int>> kept;
  for (size_t i = 0; i < count; ++i) {
    kept.push_back(std::make_shared<int>(static_cast<int>(i)));
    v.emplace_back(kept.back());  // forces several reallocations
  }

  ASSERT_EQ(v.size(), count);
  for (size_t i = 0; i < count; ++i) {
    const auto* slot = v[i];
    ASSERT_NE(slot, nullptr) << "slot " << i << " was lost on reallocation";
    EXPECT_EQ(**slot, static_cast<int>(i));
    EXPECT_EQ(slot->use_count(), 2) << "slot " << i;
  }
}

TEST(vector_of_optional_regression, DestructorReleasesElementsPastIndexEight) {
  std::vector<std::shared_ptr<int>> kept;
  {
    misc::vector_of_optional<std::shared_ptr<int>> v;
    v.reserve(32);
    for (size_t i = 0; i < 20; ++i) {
      kept.push_back(std::make_shared<int>(static_cast<int>(i)));
      v.emplace_back(kept.back());
    }
    for (const auto& p : kept) ASSERT_EQ(p.use_count(), 2);
  }

  for (size_t i = 0; i < kept.size(); ++i) {
    EXPECT_EQ(kept[i].use_count(), 1)
        << "element " << i << " was never destroyed";
  }
}

// Regression test for: operator=(const vector_of_optional&) called
// copy_to_empty_arr(), a function that doesn't exist on this class (the
// static is copy_to_empty_storages) and that wouldn't have copied
// curr_size in any case.
TEST(vector_of_optional_regression, CopyAssignment) {
  misc::vector_of_optional<std::string> a;
  a.emplace_back("one");
  a.resize(3);  // slot 1 disengaged
  a.emplace_at(2, "three");

  misc::vector_of_optional<std::string> b;
  b.emplace_back("something else");

  b = a;

  ASSERT_EQ(b.size(), 3u);
  ASSERT_NE(b[0], nullptr);
  EXPECT_EQ(*b[0], "one");
  EXPECT_EQ(b[1], nullptr);
  ASSERT_NE(b[2], nullptr);
  EXPECT_EQ(*b[2], "three");
  EXPECT_TRUE(a == b);

  // A deep copy: mutating the source must not touch the copy.
  a.emplace_at(0, "changed");
  EXPECT_EQ(*b[0], "one");
}

TEST(vector_of_optional_regression, SelfCopyAssignmentIsANoOp) {
  misc::vector_of_optional<std::string> a;
  a.emplace_back("one");

  const auto& alias = a;
  a = alias;

  ASSERT_EQ(a.size(), 1u);
  ASSERT_NE(a[0], nullptr);
  EXPECT_EQ(*a[0], "one");
}

TEST(vector_of_optional_regression, MoveAssignmentLeavesTheSourceEmpty) {
  auto kept = std::make_shared<int>(7);

  misc::vector_of_optional<std::shared_ptr<int>> a;
  a.emplace_back(kept);

  misc::vector_of_optional<std::shared_ptr<int>> b;
  b.emplace_back(std::make_shared<int>(1));

  b = std::move(a);

  ASSERT_EQ(b.size(), 1u);
  ASSERT_NE(b[0], nullptr);
  EXPECT_EQ(**b[0], 7);
  EXPECT_EQ(kept.use_count(), 2);
}

TEST(vector_of_optional_regression, CopyConstructorPreservesSize) {
  misc::vector_of_optional<int> a;
  a.emplace_back(1);
  a.resize(4);
  a.emplace_at(3, 4);

  const misc::vector_of_optional<int> b(a);

  ASSERT_EQ(b.size(), 4u);
  ASSERT_NE(b[0], nullptr);
  EXPECT_EQ(*b[0], 1);
  EXPECT_EQ(b[1], nullptr);
  EXPECT_EQ(b[2], nullptr);
  ASSERT_NE(b[3], nullptr);
  EXPECT_EQ(*b[3], 4);
}

// Regression test for: resize() grew via maybe_reallocate_and_copy_exact(s),
// which reallocates to *exactly* s even when the existing capacity is
// larger. Growing a vector that still has spare capacity therefore shrank
// the allocation while the copy loop kept running over the old (bigger)
// element count -- a write past the end of the new block.
TEST(vector_of_optional_regression, ResizeUpWithSpareCapacity) {
  misc::vector_of_optional<std::string> v;
  v.reserve(16);
  v.emplace_back("a");
  v.emplace_back("b");
  const auto capacity_before = v.capacity();

  v.resize(5);

  EXPECT_EQ(v.size(), 5u);
  EXPECT_EQ(v.capacity(), capacity_before) << "resize() shrank the allocation";
  ASSERT_NE(v[0], nullptr);
  EXPECT_EQ(*v[0], "a");
  ASSERT_NE(v[1], nullptr);
  EXPECT_EQ(*v[1], "b");
  EXPECT_EQ(v[4], nullptr);
}

// Regression test for: erase() finished by calling reset_bit(size()), one
// bit past the last element -- and the element that had been at size() - 1
// was already cleared by the shifting loop.
TEST(vector_of_optional_regression, EraseShiftsTheTailDown) {
  misc::vector_of_optional<int> v;
  for (int i = 0; i < 4; ++i) v.emplace_back(i);

  v.erase(1);

  ASSERT_EQ(v.size(), 3u);
  ASSERT_NE(v[0], nullptr);
  EXPECT_EQ(*v[0], 0);
  ASSERT_NE(v[1], nullptr);
  EXPECT_EQ(*v[1], 2);
  ASSERT_NE(v[2], nullptr);
  EXPECT_EQ(*v[2], 3);
}

TEST(vector_of_optional_regression, EraseTheLastElement) {
  misc::vector_of_optional<int> v;
  v.emplace_back(0);
  v.emplace_back(1);

  v.erase(1);

  ASSERT_EQ(v.size(), 1u);
  ASSERT_NE(v[0], nullptr);
  EXPECT_EQ(*v[0], 0);
}

TEST(vector_of_optional_regression, EraseReleasesTheElement) {
  auto kept = std::make_shared<int>(1);
  misc::vector_of_optional<std::shared_ptr<int>> v;
  v.emplace_back(kept);
  v.emplace_back(std::make_shared<int>(2));
  ASSERT_EQ(kept.use_count(), 2);

  v.erase(0);

  EXPECT_EQ(kept.use_count(), 1);
  ASSERT_EQ(v.size(), 1u);
  EXPECT_EQ(**v[0], 2);
}

// Regression test for: fill(std::nullopt) delegated to clear(), which also
// sets curr_size to 0. Filling a container should change its contents, not
// its size.
TEST(vector_of_optional_regression, FillWithNulloptKeepsTheSize) {
  misc::vector_of_optional<int> v;
  for (int i = 0; i < 3; ++i) v.emplace_back(i);

  v.fill(std::nullopt);

  EXPECT_EQ(v.size(), 3u);
  EXPECT_EQ(v[0], nullptr);
  EXPECT_EQ(v[1], nullptr);
  EXPECT_EQ(v[2], nullptr);
}

TEST(vector_of_optional_regression, FillWithAValue) {
  misc::vector_of_optional<int> v;
  v.resize(3);

  v.fill(9);

  EXPECT_EQ(v.size(), 3u);
  for (size_t i = 0; i < 3; ++i) {
    ASSERT_NE(v[i], nullptr);
    EXPECT_EQ(*v[i], 9);
  }
}

// Regression test for: operator== compared the two "is set" bitmaps
// wholesale. Those bitmaps are sized off *capacity*, so two vectors holding
// identical values compared unequal whenever their capacities differed.
TEST(vector_of_optional_regression, EqualityIgnoresCapacity) {
  misc::vector_of_optional<int> a;
  a.reserve(64);
  a.emplace_back(1);
  a.emplace_back(2);

  misc::vector_of_optional<int> b;
  b.emplace_back(1);
  b.emplace_back(2);

  ASSERT_NE(a.capacity(), b.capacity());
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(vector_of_optional_regression, EqualityComparesSize) {
  misc::vector_of_optional<int> a;
  a.emplace_back(1);

  misc::vector_of_optional<int> b;
  b.emplace_back(1);
  b.emplace_back(2);

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(vector_of_optional_regression, EqualityDistinguishesEngagedFromEmpty) {
  misc::vector_of_optional<int> a;
  a.emplace_back(0);

  misc::vector_of_optional<int> b;
  b.resize(1);  // one slot, disengaged

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(vector_of_optional_regression, EmplaceInTheMiddleWithAndWithoutRealloc) {
  // Enough capacity: takes the in-place shifting path.
  misc::vector_of_optional<std::string> v;
  v.reserve(8);
  v.emplace_back("a");
  v.emplace_back("c");
  v.emplace(1, "b");

  ASSERT_EQ(v.size(), 3u);
  EXPECT_EQ(*v[0], "a");
  EXPECT_EQ(*v[1], "b");
  EXPECT_EQ(*v[2], "c");

  // No spare capacity: takes the reallocating path.
  misc::vector_of_optional<std::string> w;
  w.emplace_back("a");
  w.emplace_back("c");
  ASSERT_EQ(w.capacity(), w.size());
  w.emplace(1, "b");

  ASSERT_EQ(w.size(), 3u);
  EXPECT_EQ(*w[0], "a");
  EXPECT_EQ(*w[1], "b");
  EXPECT_EQ(*w[2], "c");
}

TEST(vector_of_optional_regression, SwapExchangesContents) {
  misc::vector_of_optional<std::string> a;
  a.emplace_back("a");

  misc::vector_of_optional<std::string> b;
  b.emplace_back("b1");
  b.emplace_back("b2");

  a.swap(b);

  ASSERT_EQ(a.size(), 2u);
  EXPECT_EQ(*a[0], "b1");
  EXPECT_EQ(*a[1], "b2");
  ASSERT_EQ(b.size(), 1u);
  EXPECT_EQ(*b[0], "a");
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
