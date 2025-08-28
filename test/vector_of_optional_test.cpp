#include <gtest/gtest.h>
#include <misc/vector_of_optional.h>

#include <cstddef>
#include <memory>
#include <optional>

#include "test.h"

class vector_of_optionalFixture
    : public misc::vector_of_optional<std::shared_ptr<int>>,
      public testing::Test {
 public:
};

TEST_F(vector_of_optionalFixture, ConstructDefault) {
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 0);
  ASSERT_EQ(begin(), end());
}

class vector_of_optionalSingleFixture
    : public misc::vector_of_optional<std::shared_ptr<int>>,
      public testing::Test {
 public:
  const std::shared_ptr<int>* initial_emplace_result;
  vector_of_optionalSingleFixture()
      : initial_emplace_result(emplace_back(new int{10})) {}
};

TEST_F(vector_of_optionalSingleFixture, Basic) {
  ASSERT_NE(initial_emplace_result, nullptr);
  const auto& ptr_10 = *initial_emplace_result;
  ASSERT_EQ(*ptr_10, 10);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);

  ASSERT_EQ(initial_emplace_result, operator[](0));
}

TEST_F(vector_of_optionalSingleFixture, Iterators) {
  ASSERT_NE(begin(), end());

  const auto& b = *begin();
  ASSERT_NE(b, nullptr);

  const auto& r = *b;
  ASSERT_NE(r, nullptr);
  ASSERT_EQ(*r, 10);
  
  ASSERT_EQ(++begin(), end());
  ASSERT_EQ(begin()++, end());
  ASSERT_EQ(begin(), --end());
  const auto& e = end()--;
  ASSERT_EQ(e, begin());
}

TEST_F(vector_of_optionalSingleFixture, Replace) {
  const auto ptr_10 = *initial_emplace_result;
  const auto* new_emplace_result = emplace_at(0, new int{20});
  ASSERT_NE(new_emplace_result, nullptr);
  ASSERT_EQ(ptr_10.use_count(), 1);

  const auto& ptr_20 = *new_emplace_result;
  ASSERT_EQ(*ptr_20, 20);

  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);

  ASSERT_EQ(new_emplace_result, operator[](0));
}

TEST_F(vector_of_optionalSingleFixture, ResizeSmaller) {
  const auto ptr_10 = *initial_emplace_result;
  resize(0);
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(ptr_10.use_count(), 1);
}

TEST_F(vector_of_optionalSingleFixture, ResizeBigger) {
  const auto ptr_10 = *initial_emplace_result;
  resize(2);
  ASSERT_EQ(size(), 2);
  ASSERT_EQ(capacity(), 2);

  const auto* r = this->operator[](0);
  ASSERT_NE(nullptr, r);
  ASSERT_EQ(ptr_10, *r);
  ASSERT_EQ(nullptr, this->operator[](1));
}

TEST_F(vector_of_optionalSingleFixture, EmplaceClear) {
  const auto* clear_emplace_result = emplace_at(0, std::nullopt);
  ASSERT_EQ(clear_emplace_result, nullptr);
  ASSERT_EQ(operator[](0), nullptr);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
}

TEST_F(vector_of_optionalSingleFixture, EmplaceBack) {
  const auto* emplace_result = emplace_back(new int{20});
  ASSERT_NE(emplace_result, nullptr);
  const auto& ptr_20 = *emplace_result;
  ASSERT_EQ(*ptr_20, 20);

  ASSERT_EQ(size(), 2);
  ASSERT_EQ(capacity(), 2);

  const auto* front = operator[](0);
  ASSERT_NE(front, nullptr);
  ASSERT_EQ(*(*front).get(), 10);

  const auto* back = operator[](1);
  ASSERT_NE(back, nullptr);
  ASSERT_EQ(*back, ptr_20);
}

TEST_F(vector_of_optionalSingleFixture, Emplace) {
  const auto ptr_10 = *initial_emplace_result;
  const auto* emplace_result = emplace(0, new int{20});
  ASSERT_NE(emplace_result, nullptr);
  const auto& ptr_20 = *emplace_result;
  ASSERT_EQ(*ptr_20, 20);

  ASSERT_EQ(size(), 2);
  ASSERT_EQ(capacity(), 2);

  const auto* front = operator[](0);
  ASSERT_NE(front, nullptr);
  ASSERT_EQ(*(*front).get(), 20);

  const auto* back = operator[](1);
  ASSERT_NE(back, nullptr);
  ASSERT_EQ(*back, ptr_10);
}

TEST_F(vector_of_optionalSingleFixture, Reset) {
  reset(0);
  ASSERT_EQ(operator[](0), nullptr);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
}

TEST_F(vector_of_optionalSingleFixture, Erase) {
  const auto ptr_10 = *initial_emplace_result;
  erase(0);
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(ptr_10.use_count(), 1);
}

TEST_F(vector_of_optionalSingleFixture, Copy) {
  const auto ptr_10 = *initial_emplace_result;
  const misc::vector_of_optional<std::shared_ptr<int>> copy = *this;
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(copy.size(), 1);
  ASSERT_EQ(copy.capacity(), 1);

  ASSERT_EQ(ptr_10.use_count(), 3);
}

TEST_F(vector_of_optionalSingleFixture, Move) {
  const auto ptr_10 = *initial_emplace_result;
  const misc::vector_of_optional<std::shared_ptr<int>> copy = std::move(*this);
  ASSERT_EQ(copy.size(), 1);
  ASSERT_EQ(copy.capacity(), 1);

  ASSERT_EQ(ptr_10.use_count(), 2);
}

using vector_of_optionalCountingFixture = SpecMemberCountingFixture;

TEST_F(vector_of_optionalCountingFixture, DefaultConstruct) {
  misc::vector_of_optional<TestElement> v;
  EXPECT_EQ(call_counts.constructor_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, Destroy) {
  {
    misc::vector_of_optional<TestElement> v;
    v.emplace_back();
    EXPECT_EQ(call_counts.constructor_calls, 1);
  }
  EXPECT_EQ(call_counts.destructor_calls, 1);
}

TEST_F(vector_of_optionalCountingFixture, Emplace) {
  misc::vector_of_optional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.emplace_at(0);
  EXPECT_EQ(call_counts.constructor_calls, 1 + 1);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls,
            2 + 1 /* allocating new space and moving */ + 1 /* new object */);
  EXPECT_EQ(call_counts.move_constructor_calls,
            1 /* allocating new space and moving */);
  EXPECT_EQ(call_counts.destructor_calls, 1 + 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, EmplaceWithoutRealloc) {
  misc::vector_of_optional<TestElement> v;
  v.reserve(2);
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.emplace(0);
  EXPECT_EQ(call_counts.constructor_calls,
            1 + 1 /* repositioning */ + 1 /* new object*/);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, EmplaceWithRealloc) {
  misc::vector_of_optional<TestElement> v;
  v.reserve(1);
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.emplace(0);
  EXPECT_EQ(call_counts.constructor_calls,
            1 + 1 /* repositioning */ + 1 /* new object*/);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, Reserve) {
  misc::vector_of_optional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.reserve(5);
  EXPECT_EQ(call_counts.constructor_calls,
            1 + 1 /* allocating new space and moving */);
  EXPECT_EQ(call_counts.move_constructor_calls,
            1 /* allocating new space and moving */);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, Erase) {
  misc::vector_of_optional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);
  EXPECT_EQ(call_counts.destructor_calls, 0);

  v.erase(0);
  EXPECT_EQ(call_counts.constructor_calls, 1);
  EXPECT_EQ(call_counts.destructor_calls, 1);
}

TEST_F(vector_of_optionalCountingFixture, Copy) {
  misc::vector_of_optional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  const auto v2 = v;
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(vector_of_optionalCountingFixture, Move) {
  misc::vector_of_optional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  const auto v2 = std::move(v);
  EXPECT_EQ(call_counts.constructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}
