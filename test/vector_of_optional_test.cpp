#include <gtest/gtest.h>
#include <vector_of_optional.h>

#include <cstddef>
#include <memory>
#include <optional>

class VectorOfOptionalFixture
    : public misc::VectorOfOptional<std::shared_ptr<int>>,
      public testing::Test {
 public:
};

TEST_F(VectorOfOptionalFixture, ConstructDefault) {
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 0);
  ASSERT_EQ(begin(), end());
}

class VectorOfOptionalSingleFixture
    : public misc::VectorOfOptional<std::shared_ptr<int>>,
      public testing::Test {
 public:
  const std::shared_ptr<int>* initial_emplace_result;
  VectorOfOptionalSingleFixture()
      : initial_emplace_result(emplace_back(new int{10})) {}
};

TEST_F(VectorOfOptionalSingleFixture, Basic) {
  ASSERT_NE(initial_emplace_result, nullptr);
  const auto& ptr_10 = *initial_emplace_result;
  ASSERT_EQ(*ptr_10, 10);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);

  ASSERT_EQ(initial_emplace_result, operator[](0));
}

TEST_F(VectorOfOptionalSingleFixture, Iterators) {
  ASSERT_NE(begin(), end());
  ASSERT_EQ(*begin()->get(), 10);
  ASSERT_EQ(++begin(), end());
  const auto& b = begin()++;
  ASSERT_EQ(b, end());
  ASSERT_EQ(begin(), --end());
  const auto& e = end()--;
  ASSERT_EQ(e, begin());
}

TEST_F(VectorOfOptionalSingleFixture, Replace) {
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

TEST_F(VectorOfOptionalSingleFixture, ResizeSmaller) {
  const auto ptr_10 = *initial_emplace_result;
  resize(0);
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(ptr_10.use_count(), 1);
}

TEST_F(VectorOfOptionalSingleFixture, ResizeBigger) {
  const auto ptr_10 = *initial_emplace_result;
  resize(2);
  ASSERT_EQ(size(), 2);
  ASSERT_EQ(capacity(), 2);
  ASSERT_NE(nullptr, this->operator[](0));
  ASSERT_EQ(ptr_10, *this->operator[](0));
  ASSERT_EQ(nullptr, this->operator[](1));
}

TEST_F(VectorOfOptionalSingleFixture, EmplaceClear) {
  const auto* clear_emplace_result = emplace_at(0, std::nullopt);
  ASSERT_EQ(clear_emplace_result, nullptr);
  ASSERT_EQ(operator[](0), nullptr);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
}

TEST_F(VectorOfOptionalSingleFixture, EmplaceBack) {
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

TEST_F(VectorOfOptionalSingleFixture, Emplace) {
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

TEST_F(VectorOfOptionalSingleFixture, Reset) {
  reset(0);
  ASSERT_EQ(operator[](0), nullptr);
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
}

TEST_F(VectorOfOptionalSingleFixture, Erase) {
  const auto ptr_10 = *initial_emplace_result;
  erase(0);
  ASSERT_EQ(size(), 0);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(ptr_10.use_count(), 1);
}

TEST_F(VectorOfOptionalSingleFixture, Copy) {
  const auto ptr_10 = *initial_emplace_result;
  const misc::VectorOfOptional<std::shared_ptr<int>> copy = *this;
  ASSERT_EQ(size(), 1);
  ASSERT_EQ(capacity(), 1);
  ASSERT_EQ(copy.size(), 1);
  ASSERT_EQ(copy.capacity(), 1);

  ASSERT_EQ(ptr_10.use_count(), 3);
}

TEST_F(VectorOfOptionalSingleFixture, Move) {
  const auto ptr_10 = *initial_emplace_result;
  const misc::VectorOfOptional<std::shared_ptr<int>> copy = std::move(*this);
  ASSERT_EQ(copy.size(), 1);
  ASSERT_EQ(copy.capacity(), 1);

  ASSERT_EQ(ptr_10.use_count(), 2);
}

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
    [[maybe_unused]] friend bool operator!=(const TestElement& l,
                                            const TestElement& r) {
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

using VectorOfOptionalCountingFixture = SpecMemberCountingFixture;

TEST_F(VectorOfOptionalCountingFixture, DefaultConstruct) {
  misc::VectorOfOptional<TestElement> v;
  EXPECT_EQ(call_counts.constructor_calls, 0);
}

TEST_F(VectorOfOptionalCountingFixture, Destroy) {
  {
    misc::VectorOfOptional<TestElement> v;
    v.emplace_back();
    EXPECT_EQ(call_counts.constructor_calls, 1);
  }
  EXPECT_EQ(call_counts.destructor_calls, 1);
}

TEST_F(VectorOfOptionalCountingFixture, Emplace) {
  misc::VectorOfOptional<TestElement> v;
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

TEST_F(VectorOfOptionalCountingFixture, EmplaceWithoutRealloc) {
  misc::VectorOfOptional<TestElement> v;
  v.reserve(2);
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.emplace(0);
  EXPECT_EQ(call_counts.constructor_calls,
            1 + 1 /* repositioning */ + 1 /* new object*/);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(VectorOfOptionalCountingFixture, EmplaceWithRealloc) {
  misc::VectorOfOptional<TestElement> v;
  v.reserve(1);
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  v.emplace(0);
  EXPECT_EQ(call_counts.constructor_calls,
            1 + 1 /* repositioning */ + 1 /* new object*/);
  EXPECT_EQ(call_counts.destructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(VectorOfOptionalCountingFixture, Reserve) {
  misc::VectorOfOptional<TestElement> v;
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

TEST_F(VectorOfOptionalCountingFixture, Erase) {
  misc::VectorOfOptional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);
  EXPECT_EQ(call_counts.destructor_calls, 0);

  v.erase(0);
  EXPECT_EQ(call_counts.constructor_calls, 1);
  EXPECT_EQ(call_counts.destructor_calls, 1);
}

TEST_F(VectorOfOptionalCountingFixture, Copy) {
  misc::VectorOfOptional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  const auto v2 = v;
  EXPECT_EQ(call_counts.constructor_calls, 2);
  EXPECT_EQ(call_counts.copy_constructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}

TEST_F(VectorOfOptionalCountingFixture, Move) {
  misc::VectorOfOptional<TestElement> v;
  v.emplace_back();
  EXPECT_EQ(call_counts.constructor_calls, 1);

  const auto v2 = std::move(v);
  EXPECT_EQ(call_counts.constructor_calls, 1);

  EXPECT_EQ(call_counts.allocating_new_calls, 0);
}