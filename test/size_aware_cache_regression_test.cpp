#include <gtest/gtest.h>
#include <misc/size_aware_cache.h>

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace {

// A controllable clock, kept local (internal linkage via the anonymous
// namespace) so it doesn't collide at link time with the same-named clock
// in size_aware_cache_test.cpp.
struct RegressionTestClock {
  using time_point = std::chrono::steady_clock::time_point;
  using duration = std::chrono::steady_clock::duration;
  static time_point current_time;
  [[nodiscard]] static time_point now() { return current_time; }
};
RegressionTestClock::time_point RegressionTestClock::current_time{};

struct SizeCalculator {
  [[nodiscard]] size_t operator()(const std::shared_ptr<size_t>& s) {
    return *s;
  }
};

using SizeAwareCache =
    LRUCache<size_t, std::shared_ptr<size_t>, CachingStrategy::SizeAwareLRU,
             SizeCalculator, std::less<size_t>, RegressionTestClock>;

// pro_evict() is protected (eviction is normally driven by insert()), so
// reach it the way a derived class would rather than widening the public
// interface just for a test.
struct TestableCache : SizeAwareCache {
  using SizeAwareCache::pro_evict;
  using SizeAwareCache::SizeAwareCache;
};

}  // namespace

// Regression test for: pro_evict() builds a heap of per-bucket scores with
// std::make_heap and then repeatedly reads scores.back() as "the highest
// scoring bucket", evicts from it, and calls push_heap -- but it never
// calls std::pop_heap. Without that, back() is only guaranteed to be the
// true maximum when there are exactly 0, 1, or 2 non-empty buckets (with 2,
// it holds by the trivial two-node heap invariant); with 3 or more
// simultaneously non-empty buckets, back() can be an arbitrary bucket, and
// pro_evict can evict from the wrong one.
//
// This test deliberately keeps three buckets (three distinct sizes)
// non-empty at once, with one element made overwhelmingly the oldest thing
// in the cache so that -- for any scoring function that's monotonic in
// idle time, which is the whole premise of this cache -- it's unambiguously
// the one that should be evicted first.
TEST(size_aware_cache_regression, ProEvictPicksAmongThreeActiveBuckets) {
  TestableCache cache(/*capacity=*/100, /*low_watermark=*/50);
  RegressionTestClock::current_time = {};

  auto ptr_small = std::make_shared<size_t>(1);
  cache.insert({1, ptr_small});  // size 1

  auto ptr_mid = std::make_shared<size_t>(4);
  cache.insert({2, ptr_mid});  // size 4 -- this is the one that should go

  // Make the mid-sized element overwhelmingly the oldest thing in the
  // cache before anything else is inserted.
  RegressionTestClock::current_time += 1'000'000s;

  auto ptr_fresh_small = std::make_shared<size_t>(1);
  cache.insert({3, ptr_fresh_small});  // size 1, inserted just now

  auto ptr_fresh_large = std::make_shared<size_t>(16);
  cache.insert({4, ptr_fresh_large});  // size 16, inserted just now

  RegressionTestClock::current_time += 1s;

  const auto target = cache.cache_size() - 4;  // exactly what evicting only
                                               // the mid-sized bucket frees
  cache.pro_evict(target);

  EXPECT_EQ(ptr_mid.use_count(), 1)
      << "the overwhelmingly oldest element should have been evicted";
  EXPECT_EQ(ptr_small.use_count(), 2) << "unrelated bucket was touched";
  EXPECT_EQ(ptr_fresh_small.use_count(), 2)
      << "fresh element was evicted instead of the ancient one";
  EXPECT_EQ(ptr_fresh_large.use_count(), 2)
      << "fresh element was evicted instead of the ancient one";
}

namespace {
struct UnitSize {
  [[nodiscard]] size_t operator()(const std::shared_ptr<size_t>&) const {
    return 1;
  }
};

using PlainLruCache =
    LRUCache<size_t, std::shared_ptr<size_t>, CachingStrategy::LRU, UnitSize,
             std::less<size_t>, RegressionTestClock>;

struct TestablePlainCache : PlainLruCache {
  using PlainLruCache::PlainLruCache;
  using PlainLruCache::pro_evict;
};
}  // namespace

// Regression test for: both pro_evict() loops spun on `m_waterlevel >
// watermark` alone. If the waterlevel is ever above the target while the
// buckets are empty -- which is exactly the state an inconsistent
// waterlevel leaves behind -- the LRU branch dereferences an empty list's
// front() and the size-aware branch reads scores.back() off an empty
// vector. Both are infinite loops at best.
TEST(size_aware_cache_regression, EvictingAnEmptyCacheTerminates) {
  TestablePlainCache cache(/*capacity=*/10, /*low_watermark=*/5);
  cache.pro_evict(0);  // must return rather than hang or crash
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.cache_size(), 0u);
}

TEST(size_aware_cache_regression, EvictingBelowZeroStopsWhenEmpty) {
  TestablePlainCache cache(/*capacity=*/10, /*low_watermark=*/5);
  cache.insert({1, std::make_shared<size_t>(1)});
  cache.insert({2, std::make_shared<size_t>(2)});
  ASSERT_EQ(cache.cache_size(), 2u);

  cache.pro_evict(0);

  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.cache_size(), 0u);
}

TEST(size_aware_cache_regression, SizeAwareEvictionDrainsEveryBucket) {
  TestableCache cache(/*capacity=*/1000, /*low_watermark=*/500);
  RegressionTestClock::current_time = {};

  // One element in each of several size buckets.
  for (size_t i = 0; i < 6; ++i) {
    cache.insert({i, std::make_shared<size_t>(size_t{1} << i)});
    RegressionTestClock::current_time += 1s;
  }
  ASSERT_EQ(cache.size(), 6u);

  cache.pro_evict(0);

  EXPECT_EQ(cache.size(), 0u);
  EXPECT_EQ(cache.cache_size(), 0u);
}
