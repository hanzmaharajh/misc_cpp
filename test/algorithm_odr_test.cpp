#include <gtest/gtest.h>
#include <misc/algorithm.h>

#include <vector>

TEST(AlgorithmOdr, TagObjectsAreUsableFromASecondTranslationUnit) {
  EXPECT_TRUE(misc::is(2, misc::eq_any_of, 1, 2));
  EXPECT_TRUE(misc::is(2, misc::lt_all_of, 3, 4));
}

TEST(AlgorithmOdr, SetsIntersectAndIsSubset) {
  const std::vector<int> a{1, 3, 5};
  const std::vector<int> b{5, 7};
  const std::vector<int> c{2, 4};

  EXPECT_TRUE(misc::sets_intersect(a.begin(), a.end(), b.begin(), b.end()));
  EXPECT_FALSE(misc::sets_intersect(a.begin(), a.end(), c.begin(), c.end()));

  const std::vector<int> sub{1, 5};
  EXPECT_TRUE(misc::is_subset(sub.begin(), sub.end(), a.begin(), a.end()));
  EXPECT_FALSE(misc::is_subset(b.begin(), b.end(), a.begin(), a.end()));
}
