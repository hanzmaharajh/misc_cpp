#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <list>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "always_false.h"
#include "log2.h"

enum class CachingStrategy : uint8_t {
  // Basic LRU strategy
  LRU,

  // Size Aware LRU.
  // Favours evicting larger elements.
  SizeAwareLRU,

  // Size and Popularity Aware.
  // Favours evicting larger and less accessed elements.
  SizeAndPopularityAwareLRU
};

// TODO: Consider if SizeCalculator is the best approach. The size could be
// supplied on insertion instead.
//
// NOTE: SizeCalculatorType is a function object that accepts an object of type
// Value and returns its size.
template <typename Key, typename Value, CachingStrategy CacheStrategy,
          typename SizeCalculatorType, typename Compare = std::less<Key>,
          typename ClockType = std::chrono::steady_clock>
class LRUCache {
 public:
  using value_type = std::pair<Key, Value>;
  using cache_size_type = size_t;

 protected:
  // Forward declaration
  struct ElementLocator;

  // Represents a cached value and the information about it required to work the
  // caching strategy.
  //
  // Each caching strategy's elements will derive from this.
  struct BaseElement {
    Value value;
    cache_size_type size;

    using map_iterator_type =
        typename std::map<Key, ElementLocator, Compare>::iterator;
    map_iterator_type map_it;

   protected:
    BaseElement(Value v, cache_size_type s,
                typename BaseElement::map_iterator_type it)
        : value(std::move(v)), size(s), map_it(it) {}
  };

  struct LRUStrategy {
    struct Element : public BaseElement {
      Element(Value v, cache_size_type s,
              typename BaseElement::map_iterator_type it)
          : BaseElement(std::move(v), s, it) {}
      void touch() {
        // Do nothing
      }
    };

    constexpr static const size_t NUM_BUCKETS = 1;

    [[nodiscard]] static size_t get_bucket_ind(const Element&) { return 0; }
    static constexpr bool elements_change_buckets = false;
  };

  struct SizeAwareLRUStrategy {
    struct Element : public BaseElement {
      typename ClockType::time_point last_access_time;

      Element(Value v, cache_size_type s,
              typename BaseElement::map_iterator_type it)
          : BaseElement(std::move(v), s, it) {
        touch();
      }
      void touch() { last_access_time = ClockType::now(); }
    };

    // Somewhat arbitrary number of buckets.
    // 32 seems reasonable, elements must be quite large to exceed that bucket.
    constexpr static const size_t NUM_BUCKETS = 32;

    [[nodiscard]] static size_t get_bucket_ind(const Element& e) {
      // log2(0) is undefined
      if (e.size == 0) return 0;

      auto v = misc::log2(e.size);
      return std::min<size_t>(static_cast<size_t>(v), NUM_BUCKETS - 1);
    }
    static constexpr bool elements_change_buckets = true;
  };

  struct SizeAndPopularityAwareLRUStrategy {
    struct Element : public BaseElement {
      typename ClockType::time_point last_access_time;
      size_t hits = 0;

      Element(Value v, cache_size_type s,
              typename BaseElement::map_iterator_type it)
          : BaseElement(std::move(v), s, it) {
        touch();
      }

      void touch() {
        last_access_time = ClockType::now();
        ++hits;
      }
    };

    // Somewhat arbitrary number of buckets.
    // 32 seems reasonable, elements must be quite large to exceed that bucket.
    constexpr static const size_t NUM_BUCKETS = 32;

    [[nodiscard]] static size_t get_bucket_ind(const Element& e) {
      // log2(0) is undefined
      if (e.size == 0) return 0;

      // hits is always greater than 0

      // log2(e.size / e.hits) = log2(e.size) - log2(e.hits)
      auto num = misc::log2(e.size);
      auto den = misc::log2(e.hits);

      // Prevent underflow
      return std::min<size_t>(static_cast<size_t>(num > den ? num - den : 0),
                              NUM_BUCKETS - 1);
    }
    static constexpr bool elements_change_buckets = true;
  };

  // Select the element type for this type of caching strategy
  using strategy_type = std::conditional_t<
      CacheStrategy == CachingStrategy::LRU, LRUStrategy,
      std::conditional_t<CacheStrategy == CachingStrategy::SizeAwareLRU,
                         SizeAwareLRUStrategy,
                         SizeAndPopularityAwareLRUStrategy>>;

  using bucket_element_type = typename strategy_type::Element;

  // A std::list, ugh.
  // A list isn't so bad here. We will never traverse the list. We will only
  // access elements directly. It gives us stable iterators, and fast
  // insertion/deletion. We still have a lot of allocations/deallocations when
  // creating/deleting elements.
  //
  // Elements at the front of the list have been there the longest.
  using bucket_type = std::list<bucket_element_type>;

  using buckets_array_type =
      std::array<bucket_type, strategy_type::NUM_BUCKETS>;

  // Each bucket is an LRU for elements of a size range.
  buckets_array_type m_buckets;

  // A struct to help us find elements in buckets.
  struct ElementLocator {
    // The bucket index
    // NOTE: We use bucket indexes (instead of iterators) incase the cache is
    // moved (iterators would be invalidated).
    typename buckets_array_type::size_type bucket_ind;

    // Iterator into the bucket
    typename bucket_type::iterator bucket_it;
  };

  // Using std::map for its stable iterators.
  using map_type = std::map<Key, ElementLocator, Compare>;

  // Our map of keys to bucket elements.
  map_type m_keys_to_locators;

  // Object supplied by the client to compute the size of cache entries
  SizeCalculatorType m_size_calculator;

  // The current size of the cache (i.e. the sum of sizes for each element, as
  // given by the client)
  cache_size_type m_waterlevel{};

  // The "max" size of the cache. If the size ever exceeds this, the cache will
  // be drained to below the low_watermark
  //
  // NOTE: An exception to this rule is if you attempt to add an element bigger
  // than the max size. That element will live in the cache (all others will be
  // evicted).
  cache_size_type m_high_watermark;
  cache_size_type m_low_watermark;

  Value* pro_fetch(const Key& k) {
    if (const auto& map_it = m_keys_to_locators.find(k);
        map_it != std::end(m_keys_to_locators)) {
      // We found it!

      auto& [key, value] = *map_it;
      auto& bucket = m_buckets[value.bucket_ind];
      auto bucket_it = value.bucket_it;
      bucket_it->touch();

      if constexpr (strategy_type::elements_change_buckets) {
        // We must figure out its new bucket, and move it to the back of it.

        const auto new_bucket_ind = strategy_type::get_bucket_ind(*bucket_it);
        value.bucket_ind = new_bucket_ind;
        auto& new_bucket = m_buckets[new_bucket_ind];
        new_bucket.splice(std::end(new_bucket), bucket, bucket_it);
      } else {
        // Move it to the back of the bucket.
        bucket.splice(std::end(bucket), bucket, bucket_it);
      }

      return &bucket_it->value;
    } else {
      // We didn't find it.
      return nullptr;
    }
  }

  void pro_evict(size_t watermark) {
    if (watermark == 0) {
      // It's more efficient to clear(). (And the rest of procedure doesn't
      // quite work for zero.)
      clear();
      return;
    }

    if constexpr (CacheStrategy == CachingStrategy::LRU) {
      // We don't need to rank any scores with a normal LRU cache
      while (m_waterlevel > watermark) {
        auto& bucket = m_buckets[0];
        auto& element = bucket.front();
        m_waterlevel -= element.size;
        m_keys_to_locators.erase(element.map_it);
        bucket.pop_front();
      }
    } else if constexpr (CacheStrategy == CachingStrategy::SizeAwareLRU ||
                         CacheStrategy ==
                             CachingStrategy::SizeAndPopularityAwareLRU) {
      const auto& now = ClockType::now();

      struct Score {
        // Which bucket has this score
        typename buckets_array_type::iterator bucket_it;

        // The score.
        //
        // It's a duration - that doesn't make a ton of sense. But if you
        // consider the units, in the score calculation, it works.
        typename ClockType::duration score;

        // Less-than - for the heap we'll create
        bool operator<(const Score& rhs) const { return score > rhs.score; }
      };

      // Our scoring function.
      const auto& score_func = [&](auto bucket_it, const auto& element) {
        // <Time in the cache> * <group-size that the bucket holds>
        return (now - element.last_access_time) *
               (1 << std::distance(std::begin(m_buckets), bucket_it));
      };

      std::vector<Score> scores;

      for (auto bucket_it = std::begin(m_buckets);
           bucket_it != std::end(m_buckets); ++bucket_it) {
        if (!bucket_it->empty()) {
          const auto& element = bucket_it->front();
          scores.push_back({bucket_it, score_func(bucket_it, element)});
        }
      }

      std::make_heap(std::begin(scores), std::end(scores));

      while (m_waterlevel > watermark) {
        // Since we have a heap, the largest score is in the back.
        auto& score = scores.back();
        auto& bucket = *score.bucket_it;
        auto& element = bucket.front();
        m_waterlevel -= element.size;
        m_keys_to_locators.erase(element.map_it);
        bucket.pop_front();

        if (!bucket.empty()) {
          // Update the score for this element, and push it back into the heap -
          // to be re-ordered.
          const auto& next_element = bucket.front();
          score.score = score_func(score.bucket_it, next_element);
          std::push_heap(std::begin(scores), std::end(scores));
        } else {
          // This bucket is empty now, its score doesn't matter anymore.
          scores.pop_back();
        }
      }
    } else {
      static_assert(misc::always_false_v<Key>, "Missing cache strategy.");
    }
  }

 public:
  // When the summed total of sizes of elements in the cache exceed
  // high_watermark, the cache will drain elements until its size is below
  // low_watermark. The sc object is responsible for providing the size of
  // elements.
  LRUCache(cache_size_type high_watermark, cache_size_type low_watermark,
           SizeCalculatorType sc = SizeCalculatorType())
      : m_size_calculator(std::move(sc)),
        m_high_watermark(high_watermark),
        m_low_watermark(low_watermark) {}

  // No copying! (Our iterators will get all messed up.)
  LRUCache(const LRUCache&) = delete;
  LRUCache& operator=(const LRUCache&) = delete;

  // NOTE: the input parameter is not left in a consistent state. Call clear()
  // to reset it to a valid state.
  LRUCache(LRUCache&&) = default;
  // NOTE: the input parameter is not left in a consistent state. Call clear()
  // to reset it to a valid state.
  LRUCache& operator=(LRUCache&&) = default;

  const Value* fetch(const Key& k) const { return pro_fetch(k); }

  // NOTE: Although this returns a non-const pointer. The value's size will not
  // be calculated again. Take care not to alter its size.
  Value* fetch(const Key& k) { return pro_fetch(k); }

  const Value& at(const Key& k) const {
    const Value* v = fetch(k);
    if (v) return *v;
    throw std::out_of_range("Key not found in cache");
  }

  // NOTE: Although this returns a non-const ref. The value's size will not
  // be calculated again. Take care not to alter its size.
  Value& at(const Key& k) {
    Value* v = fetch(k);
    if (v) return *v;
    throw std::out_of_range("Key not found in cache");
  }

  // Returns true if inserted. False, otherwise.
  //
  // If an entry already exists, the cache is unchanged. The cached value is
  // untouched. NOTE: If an element already exists, it is not "touched".
  std::pair<Value&, bool> insert(value_type&& kv) {
    auto& [key, value] = kv;

    auto [map_it, inserted] =
        m_keys_to_locators.try_emplace(std::move(key), ElementLocator{});

    if (!inserted) {
      return {map_it->second.bucket_it->value, false};
    }

    const auto size = m_size_calculator(value);

    if (m_waterlevel + size > m_high_watermark) {
      // Make room for this element.

      // Prevent underflow
      const auto size_to_request =
          size > m_low_watermark ? 0 : m_low_watermark - size;

      pro_evict(size_to_request);
    }

    m_waterlevel += size;

    bucket_element_type be(std::move(value), size, map_it);

    const auto bucket_ind = strategy_type::get_bucket_ind(be);
    auto& bucket = m_buckets[bucket_ind];

    auto element_it = bucket.emplace(std::end(bucket), std::move(be));

    map_it->second = {bucket_ind, element_it};

    return {element_it->value, true};
  }

  std::pair<Value&, bool> insert(const value_type& kv) {
    return insert(value_type{kv});
  }

  // Returns the number of elements removed.
  //
  // NOTE: Return value can only be 1 or 0. (This follows the std::map::erase()
  // interface.)
  size_t erase(const Key& k) {
    if (const auto& map_it = m_keys_to_locators.find(k);
        map_it == std::end(m_keys_to_locators)) {
      return 0;
    } else {
      const auto& [key, value] = *map_it;

      auto& bucket = m_buckets[value.bucket_ind];
      auto bucket_it = value.bucket_it;
      m_waterlevel -= bucket_it->size;
      bucket.erase(bucket_it);
      m_keys_to_locators.erase(map_it);

      return 1;
    }
  }

  [[nodiscard]] cache_size_type cache_size() const { return m_waterlevel; }

  void clear() {
    for (auto&& bucket : m_buckets) {
      bucket.clear();
    }
    m_waterlevel = {};
  }

  [[nodiscard]] size_t size() const {
    assert(std::size(m_keys_to_locators) ==
           (std::accumulate(
               std::begin(m_buckets), std::end(m_buckets), 0u,
               [](size_t i, const auto& l) { return i + std::size(l); })));
    return std::size(m_keys_to_locators);
  }
};
