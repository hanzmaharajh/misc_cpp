#include <array_of_optional.h>
#include <dense_index_map.h>
#include <gtest/gtest.h>

#include <optional>
#include <string_view>

namespace {

struct SpecMemberCountingFixture : public testing::Test {
  static struct CallCounter {
    size_t constructor_calls = 0;
    size_t move_constructor_calls = 0;
    size_t copy_constructor_calls = 0;
    size_t destructor_calls = 0;
    size_t allocating_new_calls = 0;
    size_t placement_new_calls = 0;
  } call_counts;

  struct TestElement {
    size_t v;
    TestElement() : v{0} { ++call_counts.constructor_calls; }
    TestElement(size_t val) : v{val} { ++call_counts.constructor_calls; }
    TestElement(const TestElement& t) : v{t.v} {
      ++call_counts.constructor_calls;
      ++call_counts.copy_constructor_calls;
    }
    TestElement(TestElement&& t) : v{t.v} {
      ++call_counts.constructor_calls;
      ++call_counts.move_constructor_calls;
    }
    ~TestElement() { ++call_counts.destructor_calls; }
    void* operator new(size_t size) {
      ++call_counts.allocating_new_calls;
      return ::operator new(size);
    }
    void* operator new(std::size_t size, void* b) {
      ++call_counts.placement_new_calls;
      return ::operator new(size, b);
    }
    friend bool operator==(const TestElement& l, const TestElement& r) {
      return l.v == r.v;
    }
    friend bool operator!=(const TestElement& l, const TestElement& r) {
      return !(l == r);
    }
  };

  SpecMemberCountingFixture() { call_counts = CallCounter{}; }

  ~SpecMemberCountingFixture() {
    EXPECT_EQ(call_counts.allocating_new_calls, 0);
    EXPECT_EQ(call_counts.constructor_calls, call_counts.destructor_calls);
  }
};
SpecMemberCountingFixture::CallCounter SpecMemberCountingFixture::call_counts{};

}  // namespace

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

    ASSERT_NE(arr[3], nullptr);
    EXPECT_EQ(arr[3]->v, 0);

    arr.emplace(4, size_t{7});

    ASSERT_NE(arr[4], nullptr);
    EXPECT_EQ(arr[4]->v, 7);
  }
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.placement_new_calls, 2);
}

TEST_F(StaticArrCountingFixture, Fill) {
  misc::ArrayOfOptional<TestElement, 10> arr;
  arr.fill(TestElement(5));
  for (size_t i = 0; i < arr.size(); ++i) {
    ASSERT_NE(arr[i], nullptr);
    EXPECT_EQ(arr[i]->v, 5);
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
    ASSERT_NE(*i, nullptr);
    EXPECT_EQ((*i)->v, ind++);
  }
}

TEST_F(StaticArrSingleElementFixture_3_7, ReFill) {
  arr.fill(TestElement(5));
  for (size_t i = 0; i < arr.size(); ++i) {
    ASSERT_NE(arr[i], nullptr);
    EXPECT_EQ(arr[i]->v, 5);
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
  ASSERT_NE(arr[3], nullptr);
  EXPECT_EQ(arr[3]->v, 11);
  EXPECT_EQ(call_counts.constructor_calls, 2);
}

TEST_F(StaticArrSingleElementFixture_3_7, Erase) {
  arr.erase(3);

  EXPECT_EQ(arr[3], nullptr);
  EXPECT_EQ(call_counts.constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, CopyConstruct) {
  auto arr_copy = arr;

  ASSERT_NE(arr_copy[3], nullptr);
  EXPECT_EQ(arr_copy[3]->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, CopyAssign) {
  misc::ArrayOfOptional<TestElement, 10> arr_copy;
  arr_copy = arr;

  ASSERT_NE(arr_copy[3], nullptr);
  EXPECT_EQ(arr_copy[3]->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, MoveConstruct) {
  auto arr_copy = std::move(arr);

  ASSERT_NE(arr_copy[3], nullptr);
  EXPECT_EQ(arr_copy[3]->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.move_constructor_calls, 1);
}

TEST_F(StaticArrSingleElementFixture_3_7, MoveAssign) {
  misc::ArrayOfOptional<TestElement, 10> arr_copy;
  arr_copy = std::move(arr);

  ASSERT_NE(arr_copy[3], nullptr);
  EXPECT_EQ(arr_copy[3]->v, 7);
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.move_constructor_calls, 1);
}