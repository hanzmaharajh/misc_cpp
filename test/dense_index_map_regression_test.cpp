#include <gtest/gtest.h>
#include <misc/dense_index_map.h>

#include <memory>
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

// Regression test for: in the dense_dynamic_index_map<size_t, Value,
// identity> specialization, const_iterator::operator->() builds its return
// value as
//   wrapper{ std::pair{ind, arr.base_type::operator[](ind)} }
// -- missing both the dereference of the returned pointer and the
// explicit pair<const Key, const Value&> type that the non-const
// operator->() uses. This constructs a pair<size_t, const Value*> where a
// pair<const Key, const Value&> is expected, which shouldn't compile.
TEST(dense_index_map_regression, ConstIteratorArrow) {
  misc::dense_dynamic_index_map<size_t, std::shared_ptr<int>> m;
  m.emplace(2, std::make_shared<int>(2));

  const auto& const_map = m;
  auto it = const_map.find(2);
  ASSERT_NE(it, const_map.end());
  EXPECT_EQ((*it).first, 2u);
  ASSERT_NE((*it).second, nullptr);
  EXPECT_EQ(*(*it).second, 2);
}

TEST(dense_index_map_regression, ConstIteratorStarStillWorks) {
  // operator*() const already dereferences correctly; this is here mainly
  // so a future change to operator*() and operator->() can be compared.
  misc::dense_dynamic_index_map<size_t, std::shared_ptr<int>> m;
  m.emplace(2, std::make_shared<int>(2));

  const auto& const_map = m;
  auto it = const_map.find(2);
  ASSERT_NE(it, const_map.end());
  EXPECT_EQ((*it).first, 2u);
  EXPECT_EQ(*(*it).second, 2);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
