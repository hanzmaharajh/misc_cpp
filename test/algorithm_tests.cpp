#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm.hpp>
#include <algorithm>
#include <array>
#include <functional>
#include <vector>

TEST(sort, sort) {
  ::testing::MockFunction<int(int)> inverse;
  ON_CALL(inverse, Call).WillByDefault([](const auto& val) { return -val; });

  {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    std::array arr{4, 3, 2, 1, 0, 9, 8, 7, 6, 5};

    EXPECT_CALL(inverse, Call).Times(arr.size());
    misc::sort_cache_key(arr.begin(), arr.end(), inverse.AsStdFunction());

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

  const auto min_it = misc::min_element_cache_key(vec.begin(), vec.end(),
                                                  identity.AsStdFunction());
  EXPECT_EQ(min_it, std::min_element(vec.begin(), vec.end()));
}

INSTANTIATE_TEST_SUITE_P(
    min, VectorMinMaxTest,
    testing::Values(std::vector<int>{}, std::vector<int>{2, 3, 4, 5, 0, 1, 7}));

TEST_P(VectorMinMaxTest, minmax) {
  const auto& vec = GetParam();

  EXPECT_CALL(identity, Call).Times(static_cast<int>(vec.size()));

  const auto minmax_its = misc::minmax_element_cache_key(
      vec.begin(), vec.end(), identity.AsStdFunction());
  EXPECT_EQ(minmax_its, std::minmax_element(vec.begin(), vec.end()));
}

INSTANTIATE_TEST_SUITE_P(
    minmax, VectorMinMaxTest,
    testing::Values(std::vector<int>{}, std::vector<int>{2, 3, 4, 5, 0, 1, 7}));
