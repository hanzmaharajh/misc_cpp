#include <comp_element.h>
#include <gtest/gtest.h>

TEST(CompElement, Comp) {
  using Type = std::tuple<size_t, size_t>;

  Type l{2, 1};
  {
    Type r{1, 2};
    ASSERT_FALSE(misc::comp_element<0>{}(l, r));
    ASSERT_TRUE(misc::comp_element<1>{}(l, r));
  }
  {
    Type r{2, 1};
    ASSERT_FALSE(misc::comp_element<0>{}(l, r));
    ASSERT_FALSE(misc::comp_element<1>{}(l, r));
  }
  {
    Type r{3, 0};
    ASSERT_TRUE(misc::comp_element<0>{}(l, r));
    ASSERT_FALSE(misc::comp_element<1>{}(l, r));
  }
}

TEST(CompElements, Comp) {
  using Type = std::tuple<size_t, size_t>;

  Type l{2, 1};
  {
    Type r{1, 2};
    ASSERT_FALSE((misc::comp_elements<std::less<>, 0, 1>{}(l, r)));
    ASSERT_TRUE((misc::comp_elements<std::less<>, 1, 0>{}(l, r)));
  }
  {
    Type r{2, 1};
    ASSERT_FALSE((misc::comp_elements<std::less<>, 0, 1>{}(l, r)));
    ASSERT_FALSE((misc::comp_elements<std::less<>, 1, 0>{}(l, r)));
  }
  {
    Type r{3, 0};
    ASSERT_TRUE((misc::comp_elements<std::less<>, 0, 1>{}(l, r)));
    ASSERT_FALSE((misc::comp_elements<std::less<>, 1, 0>{}(l, r)));
  }
}
