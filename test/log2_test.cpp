#include <gtest/gtest.h>
#include <log2.h>

TEST(IntegerLog2, Singles) {
  // This test isn't parameterized because parameterization of 1,000,000 tests
  // takes too long
  for (uint64_t i = 1; i < 1'000'000; ++i) {
    EXPECT_EQ(misc::log2(i), static_cast<uint64_t>(std::log2(i)));
  }
}

class IntegerLog2Fixture : public ::testing::TestWithParam<uint64_t> {};

TEST_P(IntegerLog2Fixture, Log2) {
  uint64_t i = GetParam();
  EXPECT_EQ(misc::log2(i), static_cast<uint64_t>(std::log2(i)));
}

const auto powers_of_2 = [] {
  // Only up to 45 because the floating point log2 gets a little dodgy for big
  // numbers
  std::array<uint64_t, 45> v;
  std::generate_n(std::begin(v), std::size(v),
                  [i = 1]() mutable { return uint64_t{1} << i++; });
  return v;
}();

INSTANTIATE_TEST_SUITE_P(Powers, IntegerLog2Fixture,
                         ::testing::ValuesIn(std::begin(powers_of_2),
                                             std::end(powers_of_2)));

const auto powers_of_2_plus_1 = [] {
  auto v = powers_of_2;
  std::transform(std::begin(v), std::end(v), std::begin(v),
                 [](const auto& i) { return i + 1; });
  return v;
}();

INSTANTIATE_TEST_SUITE_P(PowersPlus1, IntegerLog2Fixture,
                         ::testing::ValuesIn(std::begin(powers_of_2_plus_1),
                                             std::end(powers_of_2_plus_1)));

const auto powers_of_2_minus_1 = [] {
  auto v = powers_of_2;
  std::transform(std::begin(v), std::end(v), std::begin(v),
                 [](const auto& i) { return i - 1; });
  return v;
}();

INSTANTIATE_TEST_SUITE_P(PowersMinus1, IntegerLog2Fixture,
                         ::testing::ValuesIn(std::begin(powers_of_2_minus_1),
                                             std::end(powers_of_2_minus_1)));
