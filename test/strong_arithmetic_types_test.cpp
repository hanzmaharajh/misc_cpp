#include <gtest/gtest.h>
#include <misc/strong_arithmetic_types.h>

#include <functional>
#include <type_traits>

namespace {
using Meters =
    misc::strong_arithmetic_type<struct MetersTag, int, misc::Addable,
                                 misc::Subtractable, misc::Incrementable,
                                 misc::Decrementable, misc::Multipliable,
                                 misc::Dividable>;
}  // namespace

TEST(strong_arithmetic_types, Addable) {
  Meters a{3};
  Meters b{4};
  EXPECT_EQ((a + b).underlying_value(), 7);

  a += b;
  EXPECT_EQ(a.underlying_value(), 7);
}

TEST(strong_arithmetic_types, Subtractable) {
  Meters a{10};
  Meters b{4};
  EXPECT_EQ((a - b).underlying_value(), 6);

  a -= b;
  EXPECT_EQ(a.underlying_value(), 6);
}

TEST(strong_arithmetic_types, PrefixIncrementReturnsNewValue) {
  Meters a{5};
  Meters& r = ++a;
  EXPECT_EQ(a.underlying_value(), 6);
  EXPECT_EQ(r.underlying_value(), 6);
  EXPECT_EQ(&r, &a);
}

// Regression test for: Incrementable::operator++(int) captures
//   Derived& d = *static_cast<Derived*>(this);
// (a *reference*, not a copy) before incrementing *this, so the returned
// "old" value is actually already-incremented -- postfix must return the
// pre-increment value.
TEST(strong_arithmetic_types, PostfixIncrementReturnsOldValue) {
  Meters a{5};
  Meters old = a++;
  EXPECT_EQ(a.underlying_value(), 6);
  EXPECT_EQ(old.underlying_value(), 5);
}

TEST(strong_arithmetic_types, PrefixDecrementReturnsNewValue) {
  Meters a{5};
  Meters& r = --a;
  EXPECT_EQ(a.underlying_value(), 4);
  EXPECT_EQ(r.underlying_value(), 4);
  EXPECT_EQ(&r, &a);
}

// Regression test for: Decrementable::operator--(int) calls `++*this;`
// instead of `--*this;` -- postfix x-- currently increments x instead of
// decrementing it (on top of the same reference-vs-copy issue as
// Incrementable's postfix operator).
TEST(strong_arithmetic_types, PostfixDecrementReturnsOldValueAndDecrements) {
  Meters a{5};
  Meters old = a--;
  EXPECT_EQ(a.underlying_value(), 4)
      << "postfix operator-- incremented instead of decrementing";
  EXPECT_EQ(old.underlying_value(), 5);
}

TEST(strong_arithmetic_types, Multipliable) {
  Meters a{3};
  EXPECT_EQ((a * 4).underlying_value(), 12);
  EXPECT_EQ((4 * a).underlying_value(), 12);

  a *= 5;
  EXPECT_EQ(a.underlying_value(), 15);
}

TEST(strong_arithmetic_types, DividableByScalar) {
  Meters a{12};
  EXPECT_EQ((a / 4).underlying_value(), 3);

  a /= 3;
  EXPECT_EQ(a.underlying_value(), 4);
}

// Regression test for: Dividable's ratio operator -- a / b on two
// same-tagged values -- used to be spelled `operator*`, so multiplying two
// Meters silently divided them.
TEST(strong_arithmetic_types, DividingTwoTaggedValuesYieldsTheBareRatio) {
  Meters a{10};
  Meters b{2};

  const auto ratio = a / b;
  static_assert(std::is_same_v<decltype(ratio), const int>,
                "dividing out the tag should leave the underlying type");
  EXPECT_EQ(ratio, 5);
}

// Multiplying two tagged values is meaningless (metres * metres isn't
// metres), so it must not compile -- and in particular it must not resolve
// to Dividable's ratio operator.
TEST(strong_arithmetic_types, MultiplyingTwoTaggedValuesIsNotAllowed) {
  static_assert(!std::is_invocable_v<std::multiplies<>, Meters, Meters>,
                "Meters * Meters should not be a valid expression");
}
