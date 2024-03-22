#include <algorithm.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <vector>

TEST(sort, sort) {
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
  std::vector<int> out;
  const auto end = misc::transform_if(
      in.begin(), in.end(), std::back_inserter(out),
      [](const int& i) { return i % 2 == 0; },
      [](const int& i) { return i + 10; });

  EXPECT_EQ(end, in.end());
  EXPECT_EQ(out, (std::vector{10, 12, 14, 16, 18}));
}
