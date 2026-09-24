#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/iterator.h>

#include <array>
#include <vector>

// Regression test for: limit_back_insert_iterator::operator++(int) does
// `return this;` where the function is declared to return the iterator by
// value (not a pointer) -- should be `return *this;` and shouldn't compile
// as written.
TEST(iterator_regression, LimitBackInsertIteratorPostIncrement) {
  std::vector<size_t> out;
  misc::limit_back_insert_iterator it{3, out};

  auto prev = it++;
  *prev = 1;
  *it = 2;

  EXPECT_THAT(out, ::testing::ElementsAre(1, 2));
}

// Regression test for: limit_output_iterator guards *advancing* past its
// end (operator++ is a no-op once begin==end), but operator*() always
// unconditionally returns *begin -- so once the destination is "full",
// every further assignment through the iterator still writes to *end(),
// one element past the declared valid range, instead of being discarded.
//
// This test owns an explicit sentinel slot immediately after the declared
// end() so the out-of-bounds write is observed deterministically rather
// than relying on undefined behavior against a container's real bounds.
TEST(iterator_regression, LimitOutputIteratorDoesNotWritePastDeclaredEnd) {
  std::vector<size_t> in{1, 2, 3, 4, 5};
  std::array<size_t, 4> out{};

  std::copy(in.begin(), in.end(),
            misc::limit_output_iterator{out.begin(), out.begin() + 3});

  EXPECT_EQ(out[0], 1u);
  EXPECT_EQ(out[1], 2u);
  EXPECT_EQ(out[2], 3u);
  EXPECT_EQ(out[3], 0u) << "wrote past the iterator's declared end()";
}

#include <list>
#include <string>

TEST(iterator_regression, LimitOutputIteratorStopsAtExactlyTheLimit) {
  std::vector<size_t> in{1, 2, 3, 4, 5};
  std::array<size_t, 3> out{};

  const auto result =
      std::copy(in.begin(), in.end(),
                misc::limit_output_iterator{out.begin(), out.end()});
  (void)result;

  EXPECT_THAT(out, ::testing::ElementsAre(1u, 2u, 3u));
}

TEST(iterator_regression, LimitOutputIteratorWithAnEmptyRange) {
  std::vector<size_t> in{1, 2, 3};
  std::array<size_t, 2> out{7, 8};

  std::copy(in.begin(), in.end(),
            misc::limit_output_iterator{out.begin(), out.begin()});

  EXPECT_THAT(out, ::testing::ElementsAre(7u, 8u))
      << "nothing should have been written";
}

TEST(iterator_regression, LimitBackInsertIteratorStopsAtTheLimit) {
  std::vector<size_t> in{1, 2, 3, 4, 5};
  std::vector<size_t> out;

  std::copy(in.begin(), in.end(), misc::limit_back_insert_iterator{0, out});
  EXPECT_TRUE(out.empty());

  std::copy(in.begin(), in.end(), misc::limit_back_insert_iterator{10, out});
  EXPECT_THAT(out, ::testing::ElementsAre(1u, 2u, 3u, 4u, 5u));
}

TEST(iterator_regression, ChainIteratorEqualityAcrossRanges) {
  std::vector<int> a{1, 2};
  std::list<int> b{3};

  auto it = misc::chain_iterator{a.begin(), a.end(), b.begin(), b.end()};
  const auto end = misc::chain_iterator{a.end(), a.end(), b.end(), b.end()};

  EXPECT_NE(it, end);
  ++it;
  ++it;
  EXPECT_NE(it, end);
  ++it;
  EXPECT_EQ(it, end);
}
