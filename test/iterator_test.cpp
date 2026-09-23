#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/iterator.h>

#include <list>
#include <vector>

TEST(Iterator, LimitBackInsertIterator) {
  std::vector<size_t> in{1, 2, 3, 4, 5};
  std::vector<size_t> out;

  std::copy(in.begin(), in.end(), misc::limit_back_insert_iterator{3, out});

  ASSERT_THAT(out, ::testing::ElementsAre(1, 2, 3));
}

TEST(Iterator, LimitOutputIterator) {
  std::vector<size_t> in{1, 2, 3, 4, 5};
  std::array<size_t, 3> out;

  std::copy(in.begin(), in.end(),
            misc::limit_output_iterator{out.begin(), out.end()});

  ASSERT_THAT(out, ::testing::ElementsAre(1, 2, 3));
}

TEST(Iterator, ChainIteratorOut) {
  std::vector<size_t> in{1, 2, 3, 4, 5, 6, 7, 8, 9};

  std::array<size_t, 3> out1;
  std::vector<size_t> out2(3);
  std::list<size_t> out3(3);

  std::copy(in.begin(), in.end(),
            misc::chain_iterator{out1.begin(), out1.end(), out2.begin(),
                                 out2.end(), out3.begin(), out3.end()});

  ASSERT_THAT(out1, ::testing::ElementsAre(1, 2, 3));
  ASSERT_THAT(out2, ::testing::ElementsAre(4, 5, 6));
  ASSERT_THAT(out3, ::testing::ElementsAre(7, 8, 9));
}

TEST(Iterator, ChainIteratorOutTail) {
  std::vector<size_t> in{1, 2, 3, 4, 5, 6, 7, 8, 9};

  std::array<size_t, 3> out1;
  std::vector<size_t> out2(3);
  std::list<size_t> out3(3);

  std::copy(in.begin(), in.end(),
            misc::chain_iterator{out1.begin(), out1.end(), out2.begin(),
                                 out2.end(), out3.begin()});

  ASSERT_THAT(out1, ::testing::ElementsAre(1, 2, 3));
  ASSERT_THAT(out2, ::testing::ElementsAre(4, 5, 6));
  ASSERT_THAT(out3, ::testing::ElementsAre(7, 8, 9));
}

TEST(Iterator, ChainIteratorIn) {
  std::array<size_t, 3> in1{1, 2, 3};
  std::vector<size_t> in2{4, 5, 6};
  std::list<size_t> in3{7, 8, 9};

  std::vector<size_t> out;
  std::copy(misc::chain_iterator{in1.begin(), in1.end(), in2.begin(), in2.end(),
                                 in3.begin(), in3.end()},
            misc::chain_iterator{in1.end(), in1.end(), in2.end(), in2.end(),
                                 in3.end(), in3.end()},
            std::back_inserter(out));

  ASSERT_THAT(out, ::testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(Iterator, ChainIteratorInTail) {
  std::array<size_t, 3> in1{1, 2, 3};
  std::vector<size_t> in2{4, 5, 6};
  std::list<size_t> in3{7, 8, 9};

  std::vector<size_t> out;
  std::copy(misc::chain_iterator{in1.begin(), in1.end(), in2.begin(), in2.end(),
                                 in3.begin()},
            misc::chain_iterator{in1.end(), in1.end(), in2.end(), in2.end(),
                                 in3.end()},
            std::back_inserter(out));

  ASSERT_THAT(out, ::testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9));
}