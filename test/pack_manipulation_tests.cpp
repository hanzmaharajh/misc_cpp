#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <pack_manipulation.h>

TEST(pack, transform_each) {
  const auto identity = [](const auto& v) { return v; };
  const auto& [a, b] = misc::transform_each(identity, std::make_shared<int>(2),
                                            std::make_shared<int>(5));

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(pack, apply_to_each_lvalue) {
  const auto identity = [](const auto& v) { return v; };
  const auto first = std::make_shared<int>(2);
  const auto& [a, b] =
      misc::transform_each(identity, first, std::make_shared<int>(5));

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(pack, take) {
  const auto first = std::make_shared<int>(2);
  const auto& [a, b] =
      misc::take<2, 2>(1, 2, first, std::make_shared<int>(5), 3, 4);

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(pack, take_first) {
  const auto first = std::make_shared<int>(2);
  const auto& [a, b] =
      misc::take_first<2>(first, std::make_shared<int>(5), 3, 4);

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(pack, take_last) {
  const auto first = std::make_shared<int>(2);
  const auto& [a, b] =
      misc::take_last<2>(1, 2, first, std::make_shared<int>(5));

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(pack, tie) {
  auto first = std::make_shared<int>(2);
  auto second = std::make_shared<int>(5);

  std::shared_ptr<int> a;
  std::shared_ptr<int> b;
  std::tie(a, b) = misc::tie<2, 2>(a, a, first, second, b);

  EXPECT_TRUE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*b, 5);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 2);
}

namespace {
template <size_t... Inds>
auto return_indexes(std::index_sequence<Inds...>) {
  return std::make_tuple(Inds...);
}
}  // namespace

TEST(pack, make_index_sequence) {
  const auto [a, b, c, d] = return_indexes(misc::make_index_sequence<3, 4>());
  EXPECT_EQ(a, 3);
  EXPECT_EQ(b, 4);
  EXPECT_EQ(c, 5);
  EXPECT_EQ(d, 6);
}
