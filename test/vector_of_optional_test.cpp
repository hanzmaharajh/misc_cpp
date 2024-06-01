#include <dense_index_map.h>
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

#include <string>
TEST(DenseIndexMap, Blah) {
  misc::dense_dynamic_index_map<size_t, std::string> m;
  m.emplace(2, "heello");
  m[3] = "dddd";
  for ([[maybe_unused]] const auto& [i, s] : m) {
  }
  {
    const auto it = m.find(2);
    ASSERT_EQ(it->second, "heello");
  }
  {
    const auto it = m.find(3);
    ASSERT_EQ(it->second, "dddd");
  }
  {
    const auto it = m.find(5);
    ASSERT_EQ(it, m.end());
  }
}