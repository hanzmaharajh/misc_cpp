#include <array_of_optional.h>
#include <dense_index_map.h>
#include <gtest/gtest.h>

#include <optional>
#include <string_view>

#include "test.h"

using StaticArrCountingFixture = SpecMemberCountingFixture;

struct StaticArrSingleElementFixture_3_7 : public StaticArrCountingFixture {
  misc::ArrayOfOptional<TestElement, 10> arr;

  StaticArrSingleElementFixture_3_7() { arr.emplace(3, size_t{7}); }
};

TEST_F(StaticArrCountingFixture, DefaultConstruct) {
  {
    misc::ArrayOfOptional<TestElement, 10> arr;
    ASSERT_EQ(arr.size(), 10);
    for (size_t i = 0; i < arr.size(); ++i) {
      EXPECT_EQ(arr[i], nullptr);
    }
  }
  EXPECT_EQ(call_counts.constructor_calls, 0);
}

TEST_F(StaticArrCountingFixture, Emplace) {
  {
    misc::ArrayOfOptional<TestElement, 10> arr;
    arr.emplace(3);

    const auto* r3 = arr[3];
    ASSERT_NE(r3, nullptr);
    EXPECT_EQ(r3->v, 0);

    arr.emplace(4, size_t{7});

    const auto* r4 = arr[4];
    ASSERT_NE(r4, nullptr);
    EXPECT_EQ(r4->v, 7);
  }
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.placement_new_calls, 2);
}

TEST_F(StaticArrCountingFixture, Fill) {
  misc::ArrayOfOptional<TestElement, 10> arr;
  arr.fill(TestElement(5));
  for (size_t i = 0; i < arr.size(); ++i) {
    const auto* r = arr[i];
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->v, 5);
  }
  EXPECT_EQ(call_counts.constructor_calls, 1 /* Fill element */ + 10);
}

TEST_F(StaticArrCountingFixture, IterateRead) {
  misc::ArrayOfOptional<TestElement, 10> arr;
  for (size_t i = 0; i < arr.size(); ++i) {
    arr.emplace(i, i);
  }
  size_t ind = 0;
  for (auto i = arr.begin(); i != arr.end(); ++i) {
    const auto* r = *i;
    ASSERT_NE(r, nullptr);
    EXPECT_EQ((r)->v, ind++);
  }
}

TEST_F(StaticArrSingleElementFixture_3_7, ReFill) {
  arr.fill(TestElement(5));
  for (size_t i = 0; i < arr.size(); ++i) {
    const auto* r = arr[i];
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->v, 5);
  }
  EXPECT_EQ(call_counts.constructor_calls, 1       /* Initial Element */
                                               + 1 /* Fill element */
                                               + 10);
}

TEST_F(StaticArrSingleElementFixture_3_7, Clear) {
  arr.clear();

  for (size_t i = 0; i < arr.size(); ++i) {
    EXPECT_EQ(arr[i], nullptr);
  }
}

TEST_F(StaticArrSingleElementFixture_3_7, EqualTrue) {
  misc::ArrayOfOptional<TestElement, 10> arr2;
  arr2.emplace(3, size_t{7});

  EXPECT_EQ(arr, arr2);
  EXPECT_FALSE(arr != arr2);
}

TEST_F(StaticArrSingleElementFixture_3_7, EqualFalse) {
  misc::ArrayOfOptional<TestElement, 10> arr2;

  EXPECT_NE(arr, arr2);
  EXPECT_FALSE(arr == arr2);

  arr2.emplace(3, size_t{8});

  EXPECT_NE(arr, arr2);
  EXPECT_FALSE(arr == arr2);
}

TEST_F(StaticArrSingleElementFixture_3_7, ReEmplace) {
  // Replace the existing element
  arr.emplace(3, size_t{11});

  // Ensure the previous was destroyed
  EXPECT_EQ(call_counts.destructor_calls, 1);

  const auto* r = arr[3];
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->v, 11);
  EXPECT_EQ(call_counts.constructor_calls, 2);
}

TEST_F(StaticArrSingleElementFixture_3_7, Erase) {
  arr.erase(3);

  EXPECT_EQ(arr[3], nullptr);
  EXPECT_EQ(call_counts.constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, CopyConstruct) {
  auto arr_copy = arr;

  const auto* r = arr_copy[3];
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, CopyAssign) {
  misc::ArrayOfOptional<TestElement, 10> arr_copy;
  arr_copy = arr;

  const auto* r = arr_copy[3];
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, MoveConstruct) {
  auto arr_copy = std::move(arr);

  const auto* r = arr_copy[3];
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.move_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, MoveAssign) {
  misc::ArrayOfOptional<TestElement, 10> arr_copy;
  arr_copy = std::move(arr);

  const auto* r = arr_copy[3];
  ASSERT_NE(r, nullptr);
  EXPECT_EQ(r->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.move_constructor_calls, 1);
}