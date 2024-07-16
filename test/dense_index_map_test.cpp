#include <dense_index_map.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

TEST(DenseDynamicIndexMap, ConstructDefault) {
  misc::dense_dynamic_index_map<size_t, size_t> m;
  ASSERT_EQ(m.find(0), m.end());
  ASSERT_EQ(m.begin(), m.end());
}

class DenseDynamicIndexMapFixture
    : public misc::dense_dynamic_index_map<size_t, std::shared_ptr<int>>,
      public testing::Test {
 public:
  DenseDynamicIndexMapFixture() {
    emplace(1, new int{1});
    emplace(2, new int{2});
    emplace(5, new int{5});
  }
};

TEST_F(DenseDynamicIndexMapFixture, FindBegin) {
  const auto it = find(1);
  ASSERT_NE(it, end());
  ASSERT_EQ(it, begin());
  ASSERT_EQ(it->first, 1);
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 1);
  ASSERT_EQ(it->second, operator[](1));
}

TEST_F(DenseDynamicIndexMapFixture, FindLast) {
  auto it = find(5);
  ASSERT_NE(it, end());
  ASSERT_EQ(it->first, 5);
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 5);
  ASSERT_EQ(it->second, operator[](5));
  ASSERT_EQ(std::next(it), end());
}

TEST_F(DenseDynamicIndexMapFixture, NotFound) {
  auto it = find(7);
  ASSERT_EQ(it, end());
}

TEST_F(DenseDynamicIndexMapFixture, EmplaceNewMid) {
  auto [it, emplaced] = emplace(3, new int{3});
  ASSERT_EQ(emplaced, true);
  ASSERT_NE(it, begin());
  ASSERT_NE(it, end());
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 3);

  const auto next = std::next(it);
  ASSERT_EQ(next, find(5));

  const auto prev = std::prev(it);
  ASSERT_EQ(prev, find(2));
}

TEST_F(DenseDynamicIndexMapFixture, EmplaceNewEnd) {
  auto [it, emplaced] = emplace(6, new int{6});
  ASSERT_EQ(emplaced, true);
  ASSERT_NE(it, begin());
  ASSERT_NE(it, end());
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 6);

  const auto next = std::next(it);
  ASSERT_EQ(next, end());

  const auto prev = std::prev(it);
  ASSERT_EQ(prev, find(5));
}

TEST_F(DenseDynamicIndexMapFixture, EmplaceExisting) {
  auto [it, emplaced] = emplace(1, std::make_shared<int>(1000));
  ASSERT_EQ(emplaced, false);
  ASSERT_EQ(it, begin());
  ASSERT_NE(it, end());
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 1);
}

TEST_F(DenseDynamicIndexMapFixture, ReassignElement) {
  auto& el = operator[](1);
  el = std::make_shared<int>(1000);
  const auto it = find(1);
  ASSERT_NE(it, end());
  ASSERT_NE(it->second, nullptr);
  ASSERT_EQ(*it->second, 1000);
}

TEST_F(DenseDynamicIndexMapFixture, Erase) {
  auto it = find(2);
  auto next_it = erase(it);
  ASSERT_NE(next_it, find(5));
  ASSERT_EQ(find(2), end());

  auto again_it = erase(it);
  ASSERT_EQ(again_it, end());
}

TEST_F(DenseDynamicIndexMapFixture, ForwardIterate) {
  std::vector<std::pair<size_t, int>> v;
  std::transform(begin(), end(), std::back_inserter(v),
                 [](const std::pair<size_t, std::shared_ptr<int>>& p) {
                   return std::make_pair(p.first, *p.second);
                 });
  using namespace ::testing;
  ASSERT_THAT(v, ElementsAre(Pair(1, 1), Pair(2, 2), Pair(5, 5)));
}

TEST_F(DenseDynamicIndexMapFixture, ReverseIterate) {
  std::vector<std::pair<size_t, int>> v;
  std::transform(std::reverse_iterator(end()), std::reverse_iterator(begin()),
                 std::back_inserter(v),
                 [](const std::pair<size_t, std::shared_ptr<int>>& p) {
                   return std::make_pair(p.first, *p.second);
                 });
  using namespace ::testing;
  ASSERT_THAT(v, ElementsAre(Pair(5, 5), Pair(2, 2), Pair(1, 1)));
}