#include <gtest/gtest.h>
#include <misc/array_of_variant.h>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

// Regression test for: operator== calls
//   cmp_element<0>(lhs.alternatives[i], lhs.storage[i], rhs.storage[i])
// passing the aligned_storage *objects* where cmp_element expects
// `const void*` arguments -- this needs &lhs.storage[i] / &rhs.storage[i]
// and shouldn't compile as written.
TEST(array_of_variant_regression, EqualityComparesActiveAlternatives) {
  misc::array_of_variant<4, int, std::string> a;
  misc::array_of_variant<4, int, std::string> b;

  a.emplace<int>(0, 1);
  a.emplace<std::string>(1, "hello");
  b.emplace<int>(0, 1);
  b.emplace<std::string>(1, "hello");

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  b.emplace<int>(0, 2);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);

  // Different active alternative at the same slot must also compare
  // unequal, not just different values of the same alternative.
  misc::array_of_variant<4, int, std::string> c;
  c.emplace<std::string>(0, "1");
  misc::array_of_variant<4, int, std::string> d;
  d.emplace<int>(0, 1);
  EXPECT_FALSE(c == d);
}

namespace {
// Counts destructor calls so a destructor run over never-constructed
// storage shows up as a mismatch rather than as silent corruption.
struct Tracked {
  static size_t constructed;
  static size_t destroyed;

  std::string s;

  Tracked() : s("default") { ++constructed; }
  explicit Tracked(std::string v) : s(std::move(v)) { ++constructed; }
  Tracked(const Tracked& o) : s(o.s) { ++constructed; }
  Tracked(Tracked&& o) noexcept : s(std::move(o.s)) { ++constructed; }
  Tracked& operator=(const Tracked&) = default;
  Tracked& operator=(Tracked&&) = default;
  ~Tracked() { ++destroyed; }

  static void reset() { constructed = destroyed = 0; }
};
size_t Tracked::constructed = 0;
size_t Tracked::destroyed = 0;
}  // namespace

// Regression test for: the default constructor went through
// fill<0>() -> emplace<0>() -> destroy_at(), which runs alternative 0's
// destructor over storage that has never been constructed. Harmless for an
// int; undefined behaviour for anything that owns a resource.
TEST(array_of_variant_regression, DefaultConstructionDoesNotDestroyRawStorage) {
  Tracked::reset();
  {
    misc::array_of_variant<4, Tracked, int> a;
    EXPECT_EQ(Tracked::destroyed, 0u)
        << "a destructor ran before anything was constructed";
    EXPECT_EQ(Tracked::constructed, 4u);

    for (size_t i = 0; i < a.size(); ++i) {
      ASSERT_NE(a.get_if<Tracked>(i), nullptr);
      EXPECT_EQ(a.get<Tracked>(i).s, "default");
    }
  }
  EXPECT_EQ(Tracked::destroyed, Tracked::constructed);
}

// Regression test for: copy_element_to_empty_arr() reinterpret_cast a
// *const* source slot to a non-const reference (reinterpret_cast can't
// strip const), and read the source's active alternative with
// `o.alternatives[pos]` when compact_int_array only offers get(). Neither
// compiles, so the copy constructor was unusable.
TEST(array_of_variant_regression, CopyConstructionPreservesAlternatives) {
  misc::array_of_variant<3, int, std::string> a;
  a.emplace<int>(0, 7);
  a.emplace<std::string>(1, "middle");
  a.emplace<int>(2, 9);

  const misc::array_of_variant<3, int, std::string> b(a);

  EXPECT_EQ(b.get<int>(0), 7);
  EXPECT_EQ(b.get<std::string>(1), "middle");
  EXPECT_EQ(b.get<int>(2), 9);
  EXPECT_TRUE(a == b);
}

TEST(array_of_variant_regression, CopyAssignmentReplacesContents) {
  misc::array_of_variant<3, int, std::string> a;
  a.emplace<std::string>(0, "from a");

  misc::array_of_variant<3, int, std::string> b;
  b.emplace<std::string>(0, "from b");
  b.emplace<std::string>(1, "also from b");

  b = a;

  EXPECT_EQ(b.get<std::string>(0), "from a");
  EXPECT_EQ(b.get<int>(1), 0);
  EXPECT_TRUE(a == b);
}

TEST(array_of_variant_regression, MoveConstructionTransfersValues) {
  misc::array_of_variant<2, int, std::string> a;
  const std::string long_enough_to_heap_allocate(64, 'x');
  a.emplace<std::string>(0, long_enough_to_heap_allocate);

  misc::array_of_variant<2, int, std::string> b(std::move(a));

  EXPECT_EQ(b.get<std::string>(0), long_enough_to_heap_allocate);
}

// Regression test for: swap() byte-swapped the raw aligned storage. A
// std::string using its small-buffer optimisation points into itself, so a
// byte-wise swap leaves both strings pointing at the wrong object.
TEST(array_of_variant_regression, SwapHandlesSelfReferentialTypes) {
  misc::array_of_variant<2, int, std::string> a;
  a.emplace<std::string>(0, "short_a");  // fits in the SSO buffer
  a.emplace<int>(1, 1);

  misc::array_of_variant<2, int, std::string> b;
  b.emplace<int>(0, 2);
  b.emplace<std::string>(1, "short_b");

  a.swap(b);

  EXPECT_EQ(a.get<int>(0), 2);
  EXPECT_EQ(a.get<std::string>(1), "short_b");
  EXPECT_EQ(b.get<std::string>(0), "short_a");
  EXPECT_EQ(b.get<int>(1), 1);
}

TEST(array_of_variant_regression, ConstVisitSeesConstReferences) {
  misc::array_of_variant<2, int, std::string> a;
  a.emplace<int>(0, 5);
  a.emplace<std::string>(1, "s");

  const auto& const_a = a;

  size_t visited = 0;
  const_a.visit([&](size_t, const auto& value) {
    static_assert(
        std::is_const_v<std::remove_reference_t<decltype(value)>>,
        "visiting a const array must not hand out mutable references");
    ++visited;
  });
  EXPECT_EQ(visited, 2u);

  size_t strings = 0;
  const_a.visit_alternative<std::string>([&](size_t i, const std::string& v) {
    EXPECT_EQ(i, 1u);
    EXPECT_EQ(v, "s");
    ++strings;
  });
  EXPECT_EQ(strings, 1u);
}

// The alternative index is what's stored, so N alternatives need only
// enough bits for N - 1.
TEST(array_of_variant_regression, AlternativeIndexStorageIsNotOversized) {
  using two_alts = misc::array_of_variant<8, int, std::string>;
  using four_alts = misc::array_of_variant<8, int, std::string, char, double>;
  EXPECT_EQ(sizeof(two_alts::alternatives_type::storage), 1u);
  EXPECT_EQ(sizeof(four_alts::alternatives_type::storage), 2u);
}
