#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <pack_manipulation.h>
#include <test.h>

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
  misc::CopyRecorder master;

  misc::CopyRecorder first{&master};
  const auto& [a, b] = misc::take_last<2>(1, 2, first, misc::CopyRecorder{});

  EXPECT_EQ(a.master, first.master);
  EXPECT_EQ(a.master->copy_constructed, 1);
  EXPECT_EQ(a.master->move_constructed, 0);
  EXPECT_EQ(b.move_constructed, 1);
  EXPECT_EQ(b.copy_constructed, 0);
}

TEST(pack, tie) {
  misc::CopyRecorder master1;
  misc::CopyRecorder master2;

  misc::CopyRecorder first{&master1};
  misc::CopyRecorder second{&master2};

  misc::CopyRecorder a;
  misc::CopyRecorder b;
  std::tie(a, b) = misc::tie<2, 2>(a, a, first, second, b);

  EXPECT_EQ(a.master, first.master);
  EXPECT_EQ(b.master, second.master);
  EXPECT_EQ(a.master->copy_constructed, 0);
  EXPECT_EQ(a.master->copy_assigned, 1);
  EXPECT_EQ(b.master->copy_constructed, 0);
  EXPECT_EQ(b.master->copy_assigned, 1);
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

TEST(pack, repeat) {
  const auto tup = misc::repeat<3>(1, 2, 3);
  EXPECT_EQ(tup, std::make_tuple(1, 2, 3, 1, 2, 3, 1, 2, 3));
}

TEST(pack, count_copies) {
  misc::CopyRecorder master;
  const auto tup = misc::repeat<3>(misc::CopyRecorder{&master});
  EXPECT_EQ(std::get<0>(tup).copy_constructed, 1);
  EXPECT_EQ(std::get<1>(tup).copy_constructed, 1);
  EXPECT_EQ(std::get<2>(tup).copy_constructed, 1);

  EXPECT_EQ(std::get<0>(tup).move_constructed, 1);
  EXPECT_EQ(std::get<1>(tup).move_constructed, 1);
  EXPECT_EQ(std::get<2>(tup).move_constructed, 1);

  EXPECT_EQ(master.copy_constructed, 3);
  EXPECT_EQ(master.move_constructed, 1);
}
