#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/array_of_variant.h>
#include <misc/overloaded.h>

class array_of_variantFixture
    : public misc::array_of_variant<5, int, std::string, std::pair<int, int>>,
      public testing::Test {
 public:
  using pair_type = std::pair<int, int>;
  array_of_variantFixture() {
    emplace<int>(0);
    emplace<1>(1, "1");
    emplace<2>(2, 2, 3);
    emplace<pair_type>(3, 3, 4);
    emplace<int>(4, 4);
  }
};

TEST_F(array_of_variantFixture, Get) {
  EXPECT_EQ(get<int>(0), 0);
  EXPECT_EQ(get<std::string>(1), "1");
  EXPECT_EQ(get<pair_type>(2), (pair_type{2, 3}));
  EXPECT_EQ(get<pair_type>(3), (pair_type{3, 4}));
  EXPECT_EQ(get<int>(4), 4);

  {
    const auto* ptr = get_if<std::string>(1);
    ASSERT_TRUE(ptr);
    ASSERT_EQ(*ptr, "1");
  }
  {
    const auto* ptr = get_if<std::string>(0);
    ASSERT_FALSE(ptr);
  }

  EXPECT_THROW([[maybe_unused]] auto y = get<std::string>(0),
               std::bad_variant_access);
}

TEST_F(array_of_variantFixture, Reassign) {
  {
    auto& x = get<int>(0);
    x = 50;
    ASSERT_EQ(get<int>(0), 50);
  }
  {
    auto* x = get_if<std::string>(1);
    ASSERT_TRUE(x);
    const auto long_str =
        "aasdfsdfdsfdsfdsfdfdsdfsdsfddsfdsfdsffdsfdsfdsfdsdsfdfssdffdsfdsfdfd"
        "sfdsfsdfdsaa";
    *x = long_str;
    ASSERT_EQ(get<std::string>(1), long_str);
  }
  {
    emplace<std::string>(4, "hello");
    ASSERT_EQ(get<std::string>(4), "hello");
  }
}

TEST_F(array_of_variantFixture, VisitAll) {
  std::vector<std::tuple<size_t, std::variant<int, std::string, pair_type>>>
      visit_results;

  visit([&](size_t i, const auto& value) {
    visit_results.emplace_back(i, value);
  });

  ASSERT_THAT(
      visit_results,
      ::testing::ElementsAre(std::tuple{0, 0}, std::tuple{1, "1"},
                             std::tuple{2, pair_type{2, 3}},
                             std::tuple{3, pair_type{3, 4}}, std::tuple{4, 4}));
}

TEST_F(array_of_variantFixture, VisitAlternative) {
  std::vector<std::tuple<size_t, std::string>>
      visit_results;

  visit_alternative<std::string>([&](size_t i, const std::string& value) {
    visit_results.emplace_back(i, value);
  });

  ASSERT_THAT(
      visit_results,
      ::testing::ElementsAre(std::tuple{1, "1"}));
}