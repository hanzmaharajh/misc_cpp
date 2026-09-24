// Coverage for which iterators in misc/ have an `operator->` and which
// deliberately do not, plus the std::iterator_traits conformance that the
// `pointer` typedef is responsible for.
//
// Two separate things are pinned down here.
//
// 1. iterator_traits<It> is only populated when It supplies *all five* of
//    iterator_category, value_type, difference_type, pointer and reference.
//    Drop the `pointer` typedef and iterator_traits silently empties, taking
//    std::next, std::prev, std::distance and std::reverse_iterator with it.
//    Every iterator here keeps that typedef, whether or not it has an
//    operator-> to go with it.
//
// 2. operator-> is only offered where `it->m` genuinely means `(*it).m`, as
//    the standard requires. dense_index_map's iterators yield a proxy pair
//    and satisfy that via misc::arrow_proxy. The optional-slot iterators
//    yield a `const T*`, where `(*it).m` is ill-formed, so they have no
//    operator-> at all -- see DeliberatelyHaveNoArrowOperator below.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/array_of_optional.h>
#include <misc/dense_index_map.h>
#include <misc/iterator.h>
#include <misc/vector_of_optional.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// NOTE: these tests use the nullable-handle accessors (`operator[]` and
// `*iterator` return a pointer that is null for a disengaged slot). gtest's
// ASSERT_NE(p, nullptr) is not a null check GCC can follow through at -O3,
// so every subsequent dereference trips -Wnull-dereference. Same suppression
// (and same TODO) as dense_index_map_test.cpp.
// TODO Sort this out. There's an issue with the iterators.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

namespace {

struct Point {
  int x;
  int y;
};

// Fails to compile unless iterator_traits<It> is fully populated.
template <typename It>
constexpr bool traits_are_complete() {
  using traits = std::iterator_traits<It>;
  return !std::is_same_v<typename traits::iterator_category, void> &&
         !std::is_same_v<typename traits::difference_type, void> &&
         sizeof(typename traits::value_type) >= 0 &&
         sizeof(typename traits::pointer) >= 0;
}

// Detects whether an iterator supplies operator-> at all.
template <typename It, typename = void>
struct has_arrow_impl : std::false_type {};
template <typename It>
struct has_arrow_impl<
    It, std::void_t<decltype(std::declval<const It&>().operator->())>>
    : std::true_type {};

template <typename It>
constexpr bool has_arrow() {
  return has_arrow_impl<It>::value;
}

}  // namespace

// ---------------------------------------------------------------------------
// misc::arrow_proxy
// ---------------------------------------------------------------------------

TEST(arrow_proxy, KeepsTheProxiedValueAlive) {
  const auto make = [] { return misc::arrow_proxy<Point>{Point{1, 2}}; };

  // The proxy is a temporary; operator-> has to reach into storage that
  // outlives the arrow expression.
  EXPECT_EQ(make()->x, 1);
  EXPECT_EQ(make()->y, 2);
}

TEST(arrow_proxy, WorksThroughAConstProxy) {
  const misc::arrow_proxy<Point> p{Point{3, 4}};
  EXPECT_EQ(p->x, 3);
  EXPECT_EQ(p->y, 4);
}

// ---------------------------------------------------------------------------
// misc::array_of_optional::const_iterator
// ---------------------------------------------------------------------------

TEST(array_of_optional_iterator, MembersAreReachedThroughTheHandle) {
  misc::array_of_optional<Point, 4> arr;
  arr.emplace(1, Point{3, 4});

  const auto it = std::next(arr.begin());

  // `*it` is a nullable handle, so a member read spells out both steps:
  // test the handle, then go through it.
  ASSERT_NE(*it, nullptr);
  EXPECT_EQ((*it)->x, 3);
  EXPECT_EQ((*it)->y, 4);
}

// These iterators deliberately have no operator->.
//
// The standard requires `it->m` to be equivalent to `(*it).m`. Here `*it`
// is itself a `const T*`, so `(*it).m` is ill-formed and an operator->
// could only ever mean `(*it)->m` -- one level of indirection away from
// what the same spelling means on every other iterator, and skipping the
// null check that `*it` invites. A proxy return type cannot rescue this:
// operator-> is reapplied until it yields a raw pointer, so a proxy that
// knows the slot is empty still has no way to report it.
//
// std::vector<bool>::iterator makes the same call for the same reason --
// it declares a `pointer` typedef for iterator_traits and supplies no
// operator-> at all.
TEST(OptionalSlotIterators, DeliberatelyHaveNoArrowOperator) {
  static_assert(!has_arrow<misc::array_of_optional<Point, 4>::const_iterator>(),
                "operator-> here would not mean (*it).m");
  static_assert(!has_arrow<misc::vector_of_optional<Point>::const_iterator>(),
                "operator-> here would not mean (*it).m");

  // ...whereas an iterator over real objects keeps it, and there `it->m`
  // really is `(*it).m`.
  static_assert(
      has_arrow<misc::dense_dynamic_index_map<size_t, int>::const_iterator>(),
      "a proxy-pair iterator still wants operator->");
}

TEST(array_of_optional_iterator, TraitsSupportTheStandardHelpers) {
  using it_type = misc::array_of_optional<Point, 4>::const_iterator;
  static_assert(traits_are_complete<it_type>());
  static_assert(std::is_same_v<std::iterator_traits<it_type>::iterator_category,
                               std::random_access_iterator_tag>);

  misc::array_of_optional<Point, 4> arr;
  EXPECT_EQ(std::distance(arr.begin(), arr.end()), 4);
  EXPECT_EQ(std::next(arr.begin(), 4), arr.end());
  EXPECT_EQ(std::prev(arr.end(), 4), arr.begin());
}

TEST(array_of_optional_iterator, RandomAccessOperations) {
  misc::array_of_optional<int, 5> arr;
  for (size_t i = 0; i < arr.size(); ++i) {
    arr.emplace(i, static_cast<int>(i) * 10);
  }

  const auto b = arr.begin();
  EXPECT_EQ(*b[2], 20);
  EXPECT_EQ(**(b + 3), 30);
  EXPECT_EQ(**(3 + b), 30);
  EXPECT_EQ(arr.end() - b, 5);
  EXPECT_TRUE(b < arr.end());
  EXPECT_TRUE(arr.end() > b);
  EXPECT_TRUE(b <= b);
  EXPECT_TRUE(b >= b);

  auto it = b;
  it += 4;
  EXPECT_EQ(**it, 40);
  it -= 4;
  EXPECT_EQ(it, b);
}

TEST(array_of_optional_iterator, PostfixOperatorsReturnTheOldPosition) {
  misc::array_of_optional<int, 3> arr;
  arr.emplace(0, 7);

  auto it = arr.begin();
  const auto before = it++;
  EXPECT_EQ(before, arr.begin());
  EXPECT_EQ(it, arr.begin() + 1);

  const auto after = it--;
  EXPECT_EQ(after, arr.begin() + 1);
  EXPECT_EQ(it, arr.begin());
}

TEST(array_of_optional_iterator, YieldsNullForDisengagedSlots) {
  misc::array_of_optional<int, 4> arr;
  arr.emplace(2, 99);

  std::vector<int> found;
  for (const auto* v : arr) {
    found.push_back(v == nullptr ? -1 : *v);
  }
  EXPECT_THAT(found, ::testing::ElementsAre(-1, -1, 99, -1));
}

// ---------------------------------------------------------------------------
// misc::vector_of_optional::const_iterator
// ---------------------------------------------------------------------------

TEST(vector_of_optional_iterator, MembersAreReachedThroughTheHandle) {
  misc::vector_of_optional<std::string> v;
  v.emplace_back("hello");

  const auto it = v.begin();
  ASSERT_NE(*it, nullptr);
  EXPECT_EQ((*it)->size(), 5u);
  EXPECT_EQ((*it)->front(), 'h');
}

TEST(vector_of_optional_iterator, DereferencingAnEmptySlotYieldsNull) {
  misc::vector_of_optional<std::string> v;
  v.resize(2);
  v.emplace_at(1, "here");

  EXPECT_EQ(*v.begin(), nullptr);
  ASSERT_NE(*std::next(v.begin()), nullptr);
  EXPECT_EQ(*(*std::next(v.begin())), "here");
}

TEST(vector_of_optional_iterator, TraitsSupportTheStandardHelpers) {
  using it_type = misc::vector_of_optional<int>::const_iterator;
  static_assert(traits_are_complete<it_type>());

  misc::vector_of_optional<int> v;
  v.emplace_back(1);
  v.emplace_back(2);
  v.emplace_back(3);

  EXPECT_EQ(std::distance(v.begin(), v.end()), 3);
  EXPECT_EQ(std::next(v.begin(), 3), v.end());
  EXPECT_EQ(std::prev(v.end(), 3), v.begin());
  EXPECT_EQ(**std::next(v.begin()), 2);
}

TEST(vector_of_optional_iterator, WorksWithReverseIterator) {
  misc::vector_of_optional<int> v;
  v.emplace_back(1);
  v.emplace_back(2);
  v.emplace_back(3);

  std::vector<int> reversed;
  std::transform(std::reverse_iterator(v.end()),
                 std::reverse_iterator(v.begin()), std::back_inserter(reversed),
                 // The iterator yields a nullable handle, so the null case
                 // has to be spelled out -- otherwise GCC (rightly) warns
                 // about a potential null dereference.
                 [](const int* p) { return p != nullptr ? *p : -1; });

  EXPECT_THAT(reversed, ::testing::ElementsAre(3, 2, 1));
}

TEST(vector_of_optional_iterator, RandomAccessOperations) {
  misc::vector_of_optional<int> v;
  for (int i = 0; i < 5; ++i) v.emplace_back(i * 10);

  const auto b = v.begin();
  EXPECT_EQ(*b[2], 20);
  EXPECT_EQ(**(b + 3), 30);
  EXPECT_EQ(**(3 + b), 30);
  EXPECT_TRUE(b < v.end());
  EXPECT_TRUE(v.end() >= b);

  auto it = b;
  it += 4;
  EXPECT_EQ(**it, 40);
  it -= 4;
  EXPECT_EQ(it, b);
}

// ---------------------------------------------------------------------------
// misc::dense_dynamic_index_map -- the general template stores a real
// std::pair, so its iterators can hand out a real pointer.
// ---------------------------------------------------------------------------

namespace {
struct StringKeyToIndex {
  size_t operator()(const std::string& s) const { return s.size(); }
};
}  // namespace

TEST(dense_index_map_iterator, GeneralTemplateArrow) {
  misc::dense_dynamic_index_map<std::string, int, StringKeyToIndex> m;
  m.emplace("ab", 20);
  m.emplace("abcd", 40);

  auto it = m.find("ab");
  ASSERT_NE(it, m.end());
  EXPECT_EQ(it->first, "ab");
  EXPECT_EQ(it->second, 20);

  // The pointer is into the container, so writing through it sticks.
  it->second = 21;
  EXPECT_EQ(m.find("ab")->second, 21);

  const auto& const_m = m;
  auto cit = const_m.find("abcd");
  ASSERT_NE(cit, const_m.end());
  EXPECT_EQ(cit->second, 40);
}

TEST(dense_index_map_iterator, GeneralTemplateTraits) {
  using map_type =
      misc::dense_dynamic_index_map<std::string, int, StringKeyToIndex>;
  static_assert(traits_are_complete<map_type::iterator>());
  static_assert(traits_are_complete<map_type::const_iterator>());

  map_type m;
  m.emplace("a", 1);
  m.emplace("ab", 2);
  EXPECT_EQ(std::distance(m.begin(), m.end()), 2);
  EXPECT_EQ(std::next(m.find("a")), m.find("ab"));
  EXPECT_EQ(std::prev(m.find("ab")), m.find("a"));
}

// ---------------------------------------------------------------------------
// misc::dense_dynamic_index_map<size_t, ...> -- the key *is* the index, so
// there is no pair in the container and operator-> has to go through
// misc::arrow_proxy.
// ---------------------------------------------------------------------------

using SizeKeyedMap =
    misc::dense_dynamic_index_map<size_t, std::shared_ptr<int>>;

class DenseIndexMapArrowFixture : public testing::Test {
 protected:
  SizeKeyedMap m;

  DenseIndexMapArrowFixture() {
    m.emplace(1, std::make_shared<int>(1));
    m.emplace(2, std::make_shared<int>(2));
    m.emplace(5, std::make_shared<int>(5));
  }
};

TEST_F(DenseIndexMapArrowFixture, IteratorArrowYieldsAMapLikePair) {
  auto it = m.find(2);
  ASSERT_NE(it, m.end());

  EXPECT_EQ(it->first, 2u);
  ASSERT_NE(it->second, nullptr);
  EXPECT_EQ(*it->second, 2);

  // `->second` is a reference into the map, so assigning through it must be
  // visible in the map afterwards -- an arrow proxy that copied the value
  // would silently drop this.
  it->second = std::make_shared<int>(22);
  EXPECT_EQ(*m.find(2)->second, 22);
}

TEST_F(DenseIndexMapArrowFixture, ConstIteratorArrow) {
  const auto& const_m = m;
  auto it = const_m.find(5);
  ASSERT_NE(it, const_m.end());

  EXPECT_EQ(it->first, 5u);
  ASSERT_NE(it->second, nullptr);
  EXPECT_EQ(*it->second, 5);
  static_assert(std::is_const_v<std::remove_reference_t<decltype(it->second)>>,
                "a const_iterator must not hand out a mutable reference");
}

TEST_F(DenseIndexMapArrowFixture, ArrowOnAConstIteratorObject) {
  const SizeKeyedMap::iterator it = m.find(1);
  EXPECT_EQ(it->first, 1u);
  EXPECT_EQ(*it->second, 1);
}

TEST_F(DenseIndexMapArrowFixture, ArrowMatchesStar) {
  for (auto it = m.begin(); it != m.end(); ++it) {
    EXPECT_EQ(it->first, (*it).first);
    EXPECT_EQ(it->second, (*it).second);
  }
}

TEST_F(DenseIndexMapArrowFixture, TraitsSupportTheStandardHelpers) {
  static_assert(traits_are_complete<SizeKeyedMap::iterator>());
  static_assert(traits_are_complete<SizeKeyedMap::const_iterator>());
  static_assert(std::is_same_v<
                std::iterator_traits<SizeKeyedMap::iterator>::iterator_category,
                std::bidirectional_iterator_tag>);

  EXPECT_EQ(std::distance(m.begin(), m.end()), 3);
  EXPECT_EQ(std::next(m.find(1)), m.find(2));
  EXPECT_EQ(std::prev(m.find(5)), m.find(2));
}

TEST_F(DenseIndexMapArrowFixture, ReverseIterationOverProxyPairs) {
  std::vector<size_t> keys;
  for (auto it = std::reverse_iterator(m.end());
       it != std::reverse_iterator(m.begin()); ++it) {
    keys.push_back((*it).first);
  }
  EXPECT_THAT(keys, ::testing::ElementsAre(5u, 2u, 1u));
}

TEST_F(DenseIndexMapArrowFixture, PostfixIncrementReturnsTheOldPosition) {
  auto it = m.begin();
  const auto before = it++;
  EXPECT_EQ(before, m.find(1));
  EXPECT_EQ(it, m.find(2));

  const auto after = it--;
  EXPECT_EQ(after, m.find(2));
  EXPECT_EQ(it, m.find(1));
}

// Regression test for: `while (ind != size && !arr.is_set(++ind));` reads
// the "is set" bit at index `size` on the final step. When size happens to
// equal capacity that bit is one past the end of the bitmap.
TEST(dense_index_map_iterator, IncrementToEndDoesNotReadPastTheBitmap) {
  misc::dense_dynamic_index_map<size_t, int> m;
  // 8 entries fills the bitmap's first byte exactly, so index 8 is out of
  // bounds by exactly one bit.
  for (size_t i = 0; i < 8; ++i) m.emplace(i, static_cast<int>(i));

  size_t count = 0;
  for (auto it = m.begin(); it != m.end(); ++it) ++count;
  EXPECT_EQ(count, 8u);
}

// ---------------------------------------------------------------------------
// misc::chain_iterator
// ---------------------------------------------------------------------------

TEST(chain_iterator, ArrowAndConstDereference) {
  std::vector<std::string> a{"one", "two"};
  std::list<std::string> b{"three"};

  auto it = misc::chain_iterator{a.begin(), a.end(), b.begin(), b.end()};
  EXPECT_EQ(it->size(), 3u);
  EXPECT_EQ(*it, "one");

  ++it;
  ++it;
  EXPECT_EQ(*it, "three");
  EXPECT_EQ(it->size(), 5u);

  // Regression: operator*() const called the non-const prv_value(), so a
  // const chain_iterator couldn't be dereferenced at all.
  const auto const_it =
      misc::chain_iterator{a.begin(), a.end(), b.begin(), b.end()};
  EXPECT_EQ(*const_it, "one");
  EXPECT_EQ(const_it->size(), 3u);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
