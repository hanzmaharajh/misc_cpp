#include <gtest/gtest.h>
#include <misc/array_of_optional.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

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

namespace {
struct Point {
  int x;
  int y;
};
}  // namespace

// const_iterator::operator->() used to compute `&arr[ind]`, where arr[ind]
// (the const array_of_optional::operator[]) returns a `const T*` *by
// value* -- so it handed back the address of a temporary. There is no
// operator-> any more (`*it` is already the pointer, and `it->m` could
// never have meant `(*it).m`); this checks the surviving spelling reads
// the element correctly.
TEST(array_of_optional_regression, MembersAreReachedThroughTheHandle) {
  misc::array_of_optional<Point, 4> arr;
  arr.emplace(1, Point{3, 4});

  auto it = arr.begin();
  ++it;  // now pointing at index 1, the set element
  ASSERT_NE(*it, nullptr);
  EXPECT_EQ((*it)->x, 3);
  EXPECT_EQ((*it)->y, 4);
}

// Regression test for: the binary (non-compound) operator+ and operator-
// on const_iterator both build a local `retval` and return it as
// `const_iterator&` -- a reference to a variable that no longer exists once
// the function returns. (operator+ additionally fails to compile as
// written, since it calls the non-const operator+= on a `const auto`.)
TEST(array_of_optional_regression, IteratorPlusAndMinusReturnUsableValues) {
  misc::array_of_optional<int, 10> arr;
  for (size_t i = 0; i < arr.size(); ++i) {
    arr.emplace(i, static_cast<int>(i));
  }

  const auto begin = arr.begin();

  const auto it3 = begin + 3;
  ASSERT_NE(*it3, nullptr);
  EXPECT_EQ(**it3, 3);

  const auto back_to_0 = it3 - 3;
  ASSERT_NE(*back_to_0, nullptr);
  EXPECT_EQ(**back_to_0, 0);
}

// Regression test for: swap() swapped the raw aligned storage bytes. A
// std::string using its small-buffer optimisation points into itself, so a
// byte-wise swap leaves both strings pointing at the wrong object.
TEST(array_of_optional_regression, SwapHandlesSelfReferentialTypes) {
  misc::array_of_optional<std::string, 3> a;
  a.emplace(0, "short_a");  // fits in the SSO buffer
  a.emplace(2, std::string(64, 'x'));

  misc::array_of_optional<std::string, 3> b;
  b.emplace(1, "short_b");

  a.swap(b);

  EXPECT_EQ(a[0], nullptr);
  ASSERT_NE(a[1], nullptr);
  EXPECT_EQ(*a[1], "short_b");
  EXPECT_EQ(a[2], nullptr);

  ASSERT_NE(b[0], nullptr);
  EXPECT_EQ(*b[0], "short_a");
  EXPECT_EQ(b[1], nullptr);
  ASSERT_NE(b[2], nullptr);
  EXPECT_EQ(*b[2], std::string(64, 'x'));
}

TEST(array_of_optional_regression, FreeSwapIsFound) {
  misc::array_of_optional<int, 2> a;
  a.emplace(0, 1);
  misc::array_of_optional<int, 2> b;
  b.emplace(1, 2);

  using std::swap;
  swap(a, b);

  EXPECT_EQ(a[0], nullptr);
  ASSERT_NE(a[1], nullptr);
  EXPECT_EQ(*a[1], 2);
  ASSERT_NE(b[0], nullptr);
  EXPECT_EQ(*b[0], 1);
}

TEST(array_of_optional_regression, CopyAndMoveSemantics) {
  auto kept = std::make_shared<int>(5);

  misc::array_of_optional<std::shared_ptr<int>, 3> a;
  a.emplace(1, kept);
  ASSERT_EQ(kept.use_count(), 2);

  const misc::array_of_optional<std::shared_ptr<int>, 3> copy(a);
  EXPECT_EQ(kept.use_count(), 3);
  ASSERT_NE(copy[1], nullptr);
  EXPECT_EQ(**copy[1], 5);
  EXPECT_EQ(copy[0], nullptr);

  misc::array_of_optional<std::shared_ptr<int>, 3> moved(std::move(a));
  ASSERT_NE(moved[1], nullptr);
  EXPECT_EQ(**moved[1], 5);
}

TEST(array_of_optional_regression, EraseAndClearRunDestructors) {
  auto one = std::make_shared<int>(1);
  auto two = std::make_shared<int>(2);

  misc::array_of_optional<std::shared_ptr<int>, 4> a;
  a.emplace(0, one);
  a.emplace(3, two);

  EXPECT_EQ(a.erase(0), 1u);
  EXPECT_EQ(a.erase(0), 0u) << "erasing an empty slot should report nothing";
  EXPECT_EQ(one.use_count(), 1);
  EXPECT_EQ(two.use_count(), 2);

  a.clear();
  EXPECT_EQ(two.use_count(), 1);
  EXPECT_EQ(a[3], nullptr);
}

TEST(array_of_optional_regression, EqualityAccountsForWhichSlotsAreSet) {
  misc::array_of_optional<int, 3> a;
  a.emplace(0, 1);

  misc::array_of_optional<int, 3> b;
  b.emplace(0, 1);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  b.emplace(1, 2);
  EXPECT_FALSE(a == b);

  b.erase(1);
  ASSERT_TRUE(a == b);
  b.emplace(0, 99);
  EXPECT_FALSE(a == b);
}

TEST(array_of_optional_regression,
     EmplaceOverAnEngagedSlotDestroysTheOldValue) {
  auto old_value = std::make_shared<int>(1);
  misc::array_of_optional<std::shared_ptr<int>, 2> a;
  a.emplace(0, old_value);
  ASSERT_EQ(old_value.use_count(), 2);

  a.emplace(0, std::make_shared<int>(2));

  EXPECT_EQ(old_value.use_count(), 1);
  EXPECT_EQ(**a[0], 2);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
