#include <algorithm.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <boost/range.hpp>
#include <functional>
#include <iterator>
#include <vector>

#include "test.h"

TEST(transform_sort, sort) {
  ::testing::MockFunction<int(int)> inverse;
  ON_CALL(inverse, Call).WillByDefault([](const auto& val) { return -val; });

  {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    std::array arr{4, 3, 2, 1, 0, 9, 8, 7, 6, 5};

    EXPECT_CALL(inverse, Call).Times(arr.size());
    misc::transform_sort(arr.begin(), arr.end(), inverse.AsStdFunction());

    EXPECT_TRUE(std::is_sorted(arr.begin(), arr.end(), std::greater<>{}));
  }
}

struct VectorMinMaxTest : public testing::TestWithParam<std::vector<int>> {
  ::testing::MockFunction<int(int)> identity{};

  VectorMinMaxTest() {
    ON_CALL(identity, Call).WillByDefault([](const auto& val) { return val; });
  }
};

TEST_P(VectorMinMaxTest, min) {
  const auto& vec = GetParam();

  EXPECT_CALL(identity, Call).Times(static_cast<int>(vec.size()));

  const auto min_it = misc::transform_min_element(vec.begin(), vec.end(),
                                                  identity.AsStdFunction());
  EXPECT_EQ(min_it, std::min_element(vec.begin(), vec.end()));
}

INSTANTIATE_TEST_SUITE_P(
    min, VectorMinMaxTest,
    testing::Values(std::vector<int>{}, std::vector<int>{2, 3, 4, 5, 0, 1, 7}));

TEST_P(VectorMinMaxTest, minmax) {
  const auto& vec = GetParam();

  EXPECT_CALL(identity, Call).Times(static_cast<int>(vec.size()));

  const auto minmax_its = misc::transform_minmax_element(
      vec.begin(), vec.end(), identity.AsStdFunction());
  EXPECT_EQ(minmax_its, std::minmax_element(vec.begin(), vec.end()));
}

INSTANTIATE_TEST_SUITE_P(
    minmax, VectorMinMaxTest,
    testing::Values(std::vector<int>{}, std::vector<int>{2, 3, 4, 5, 0, 1, 7}));

TEST(transform_if, transform_if) {
  const std::vector in{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::array<int, 100> out;
  const auto out_it = misc::transform_if(
      in.begin(), in.end(), out.begin(),
      [](const int& i) { return i % 2 == 0; },
      [](const int& i) { return i + 10; });

  EXPECT_EQ(out_it, std::next(out.begin(), 5));
  EXPECT_THAT(
      boost::make_iterator_range(out.begin(), std::next(out.begin(), 5)),
      testing::ElementsAre(10, 12, 14, 16, 18));
}

TEST(permutations_without_replacement, permutations_without_replacement) {
  const std::vector<int> v{1, 2, 3, 4, 5};
  struct Visitor : misc::CopyRecorder {
    std::vector<std::array<int, 3>> acc;
    void operator()(int a, int b, int c) { acc.push_back(std::array{a, b, c}); }
  } in_func;

  const auto out_func = misc::visit_permutations_without_replacement<3>(
      v.begin(), v.end(), std::move(in_func));

  EXPECT_EQ(out_func.acc.size(), 60);
  EXPECT_EQ(out_func.copy_constructed, 0);
}

using AlgorithmCountingFixture = misc::SpecMemberCountingFixture;

TEST_F(AlgorithmCountingFixture, PartitionTransform) {
  std::array<TestElement, 10> vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ::testing::MockFunction<size_t(const TestElement&)> get_val;
  ON_CALL(get_val, Call).WillByDefault([](const auto& e) { return e.v; });
  EXPECT_CALL(get_val, Call).Times(static_cast<int>(vec.size()));

  misc::partition_transform(vec.begin(), vec.end(), get_val.AsStdFunction(),
                            [](size_t v) { return v % 2 == 0; });

  EXPECT_EQ(call_counts.constructor_calls, 10);

  EXPECT_THAT(
      boost::make_iterator_range(vec.begin(), std::next(vec.begin(), 5)),
      testing::UnorderedElementsAre(2, 4, 6, 8, 10));
  EXPECT_THAT(boost::make_iterator_range(std::next(vec.begin(), 5), vec.end()),
              testing::UnorderedElementsAre(1, 3, 5, 7, 9));
}