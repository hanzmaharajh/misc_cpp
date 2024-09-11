#include <gtest/gtest.h>
#include <tagged_ptr.h>

#include "test.h"

TEST(tagged_ptr, Construct) {
  if (alignof(std::pair<size_t, size_t>) != 8) GTEST_SKIP();
  auto ptr = std::make_unique<std::pair<size_t, size_t>>(1, 2);
  misc::tagged_ptr<std::pair<size_t, size_t>> tagged{ptr.get(), 0x07};

  EXPECT_EQ(tagged.get(), ptr.get());
  EXPECT_EQ(tagged->first, 1);
  EXPECT_EQ(tagged->second, 2);
  EXPECT_EQ(tagged.tag(), 0x07);
}

TEST(unique_tagged_ptr, Construct) {
  if (alignof(std::pair<size_t, size_t>) != 8) GTEST_SKIP();
  auto ptr = new std::pair<size_t, size_t>{1, 2};
  misc::unique_tagged_ptr<std::pair<size_t, size_t>> tagged{ptr, 0x07};

  EXPECT_EQ(tagged.get(), ptr);
  EXPECT_EQ(tagged->first, 1);
  EXPECT_EQ(tagged->second, 2);
  EXPECT_EQ(tagged.tag(), 0x07);
}

using UniqueTaggedPtrCountingFixture = misc::SpecMemberCountingFixture;

TEST_F(UniqueTaggedPtrCountingFixture, Destroy) {
  if (alignof(TestElement) != 8) GTEST_SKIP();
  {
    auto ptr = new TestElement{1};
    misc::unique_tagged_ptr<TestElement> tagged{ptr, 0x07};
  }
  ASSERT_EQ(call_counts.destructor_calls, 1);
}

TEST(tagged_arr, Construct) {
  if (alignof(std::pair<size_t, size_t>) != 8) GTEST_SKIP();
  auto ptr = std::make_unique<std::pair<size_t, size_t>[]>(2);
  ptr[0] = {1, 2};
  ptr[1] = {3, 4};
  misc::tagged_ptr<std::pair<size_t, size_t>[]> tagged { ptr.get(), 0x07 };

  EXPECT_EQ(tagged.get(), ptr.get());
  EXPECT_EQ(tagged[0].first, 1);
  EXPECT_EQ(tagged[0].second, 2);
  EXPECT_EQ(tagged[1].first, 3);
  EXPECT_EQ(tagged[1].second, 4);
  EXPECT_EQ(tagged.tag(), 0x07);
}

TEST(unique_tagged_arr, Construct) {
  if (alignof(std::pair<size_t, size_t>) != 8) GTEST_SKIP();
  auto ptr = new std::pair<size_t, size_t>[2] {
    {1, 2}, { 3, 4 }
  };
  misc::unique_tagged_ptr<std::pair<size_t, size_t>[]> tagged { ptr, 0x07 };

  EXPECT_EQ(tagged.get(), ptr);
  EXPECT_EQ(tagged[0].first, 1);
  EXPECT_EQ(tagged[0].second, 2);
  EXPECT_EQ(tagged[1].first, 3);
  EXPECT_EQ(tagged[1].second, 4);
  EXPECT_EQ(tagged.tag(), 0x07);
}

TEST_F(UniqueTaggedPtrCountingFixture, DestroyArr) {
  if (alignof(TestElement) != 8) GTEST_SKIP();
  {
    auto ptr = new TestElement[10]{};
    misc::unique_tagged_ptr<TestElement[]> tagged{ptr, 0x07};
  }
  ASSERT_EQ(call_counts.destructor_calls, 10);
}