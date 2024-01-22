#include <gtest/gtest.h>

#include <minmax_heap.h>

using namespace misc;

class MinMaxHeapFixture : public minmax_heap<size_t>, public testing::Test {
 public:
  bool is_heap() const {
    return is_minmax_heap(m_heap.begin(), m_heap.end(), value_comp());
  }
};

TEST_F(MinMaxHeapFixture, pop_front) {
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);

  for (size_t i = 1; i <= 16; ++i) {
    push(i);
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 1);
    EXPECT_EQ(back(), i);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), i);
  }

  for (size_t i = 0; i < 8; ++i) {
    pop_front();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), i + 2);
    EXPECT_EQ(back(), 16);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 16 - i - 1);
  }

  for (size_t i = 0; i < 7; ++i) {
    pop_back();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 9);
    EXPECT_EQ(back(), 16 - i - 1);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 8 - i - 1);
  }

  pop_back();
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);
}

TEST_F(MinMaxHeapFixture, pop_back) {
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);

  for (size_t i = 1; i <= 16; ++i) {
    push(i / 2);
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 0);
    EXPECT_EQ(back(), i / 2);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), i);
  }

  for (size_t i = 0; i < 8; ++i) {
    pop_front();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), i / 2 + 1);
    EXPECT_EQ(back(), 8);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 16 - i - 1);
  }

  for (size_t i = 0; i < 7; ++i) {
    pop_back();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 4);
    EXPECT_EQ(back(), 8 - i / 2 - 1);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 8 - i - 1);
  }

  pop_back();
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);
}

class MaxMinHeapFixture : public minmax_heap<size_t, std::greater<>>,
                          public testing::Test {
 public:
  bool is_heap() const {
    return is_minmax_heap(m_heap.begin(), m_heap.end(), value_comp());
  }
};

TEST_F(MaxMinHeapFixture, maxmin) {
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);

  for (size_t i = 1; i <= 16; ++i) {
    push(i);
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), i);
    EXPECT_EQ(back(), 1);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), i);
  }

  for (size_t i = 0; i < 8; ++i) {
    pop_front();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 16 - i - 1);
    EXPECT_EQ(back(), 1);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 16 - i - 1);
  }

  for (size_t i = 0; i < 7; ++i) {
    pop_back();
    EXPECT_TRUE(is_heap());
    EXPECT_EQ(front(), 8);
    EXPECT_EQ(back(), i + 2);
    EXPECT_FALSE(empty());
    EXPECT_EQ(size(), 8 - i - 1);
  }

  pop_back();
  EXPECT_TRUE(is_heap());
  EXPECT_TRUE(empty());
  EXPECT_EQ(size(), 0);
}

struct CopyCounter {
  size_t value;
  size_t copies = 0;
  size_t moves = 0;

  bool operator<(const CopyCounter& o) const { return value < o.value; }
  bool operator==(const CopyCounter& o) const { return value == o.value; }
  CopyCounter(size_t v) : value(v) {}
  CopyCounter(const CopyCounter& c) noexcept {
    value = c.value;
    copies = c.copies + 1;
  }
  CopyCounter(CopyCounter&& c) noexcept {
    value = c.value;
    moves = c.moves + 1;
  }
  CopyCounter& operator=(const CopyCounter& c) {
    value = c.value;
    copies = c.copies + 1;
    return *this;
  }
  CopyCounter& operator=(CopyCounter&& c) {
    value = c.value;
    moves = c.moves + 1;
    return *this;
  }
};

struct MinMaxHeapNoCopyFixture : public minmax_heap<CopyCounter>,
                                 public testing::Test {
  MinMaxHeapNoCopyFixture() {
    container_type vec;
    for (size_t i = 0; i < 16; ++i) {
      vec.emplace_back(i);
    }

    adopt_sequence(std::move(vec));
  }
};

TEST_F(MinMaxHeapNoCopyFixture, extract_seq_no_copy) {
  const auto& vec = extract_sequence();
  ASSERT_EQ(size(), 0);
  EXPECT_TRUE(is_minmax_heap(vec.begin(), vec.end()));
  EXPECT_TRUE(std::all_of(vec.begin(), vec.end(),
                          [](const auto& cc) { return cc.copies == 0; }));
}

TEST_F(MinMaxHeapNoCopyFixture, copy_construct) {
  minmax_heap copy = *this;
  ASSERT_EQ(size(), 16);
  const auto& vec = copy.extract_sequence();
  EXPECT_EQ(vec, m_heap);
  EXPECT_TRUE(std::all_of(vec.begin(), vec.end(),
                          [](const auto& cc) { return cc.copies == 1; }));
}