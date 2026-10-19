#pragma once

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <optional>
#include <utility>

#include "iterator.h"
#include "vector_of_optional.h"

namespace misc {

/// @brief A transparent function object that returns its argument
/// unchanged, used as the default key-to-index mapping.
struct identity {
  using is_transparent = void;

  /// @brief Returns @p t unchanged
  /// @tparam T The argument type
  /// @param t The value to pass through
  /// @return @p t, forwarded
  template <typename T>
  constexpr T&& operator()(T&& t) const noexcept {
    return std::forward<T>(t);
  }
};

/// @brief An associative container for keys that map onto a dense range of
/// small integers.
///
/// Rather than hashing or a tree, entries are stored in a flat array
/// indexed by KeyToIndexMap(key), with the gaps left empty. Lookup is an
/// array index, and iteration visits present entries in key order. It is
/// only worth using when the index range is not much larger than the number
/// of entries, since the storage is proportional to the largest index.
/// @tparam Key The key type
/// @tparam Value The mapped type
/// @tparam KeyToIndexMap A function object mapping a Key to its array index
template <typename Key, typename Value, typename KeyToIndexMap = identity>
class dense_dynamic_index_map
    : protected vector_of_optional<std::pair<const Key, Value>> {
  KeyToIndexMap index_map;
  using base_type = vector_of_optional<std::pair<const Key, Value>>;

 public:
  /// @brief Constructs an empty map
  /// @param init_count The number of slots to reserve up front
  /// @param m The key-to-index mapping to use
  dense_dynamic_index_map(size_t init_count = 0,
                          KeyToIndexMap m = KeyToIndexMap{})
      : index_map(std::move(m)) {
    reserve(init_count);
  }

  /// @brief A map-like iterator over the present entries, skipping the
  /// gaps between them.
  ///
  /// Each entry is a real std::pair in the container, so @c operator->
  /// hands out an ordinary pointer to it.
  struct iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value>;
    using pointer = std::pair<const Key, Value>*;
    using reference = std::pair<const Key, Value>&;
    using const_reference = const std::pair<const Key, Value>&;

    friend class dense_dynamic_index_map;

    /// @brief Constructs an iterator at or after slot @p i
    /// @param i The slot to start from. Empty slots are skipped forwards.
    /// @param map The map iterated over. It must outlive the iterator.
    iterator(size_t i, dense_dynamic_index_map& map) : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    /// @brief Returns the key/value pair at the current entry
    /// @return A reference to the stored pair
    reference operator*() { return *arr.base_type::operator[](ind); }

    /// @copydoc operator*()
    const_reference operator*() const {
      return *arr.base_type::operator[](ind);
    }

    /// @brief Accesses the current entry's key and value
    /// @return A pointer to the stored pair
    pointer operator->() { return arr.base_type::operator[](ind); }

    /// @copydoc operator->()
    pointer operator->() const { return arr.base_type::operator[](ind); }

    /// @brief Advances to the next present entry
    /// @return *this
    iterator& operator++() {
      const auto last = arr.base_type::size();
      if (ind == last) return *this;
      for (++ind; ind != last && !arr.is_set(ind); ++ind);
      return *this;
    }
    /// @brief Advances to the next present entry
    /// @return The iterator's value from before the increment
    iterator operator++(int) {
      auto retval = *this;
      ++*this;
      return retval;
    }
    /// @brief Steps back to the previous present entry
    /// @return *this
    iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind));
      return *this;
    }
    /// @brief Steps back to the previous present entry
    /// @return The iterator's value from before the decrement
    iterator operator--(int) {
      auto retval = *this;
      --*this;
      return retval;
    }

    /// @brief Compares position and map identity
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if both refer to the same entry of the same map
    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    /// @copydoc operator==()
    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    dense_dynamic_index_map& arr;
  };

  /// @copydoc iterator
  struct const_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value>;
    using pointer = const std::pair<const Key, Value>*;
    using reference = const std::pair<const Key, Value>&;

    friend class dense_dynamic_index_map;

    /// @brief Converts a mutable iterator to a const one
    /// @param it The iterator to convert
    const_iterator(iterator it) : ind(it.ind), arr(it.arr) {}
    /// @brief Constructs an iterator at or after slot @p i
    /// @param i The slot to start from. Empty slots are skipped forwards.
    /// @param map The map iterated over. It must outlive the iterator.
    const_iterator(size_t i, const dense_dynamic_index_map& map)
        : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    /// @brief Returns the key/value pair at the current entry
    /// @return A reference to the stored pair
    reference operator*() const { return *arr.base_type::operator[](ind); }

    /// @brief Accesses the current entry's key and value
    /// @return A pointer to the stored pair
    pointer operator->() const { return arr.base_type::operator[](ind); }

    /// @brief Advances to the next present entry
    /// @return *this
    const_iterator& operator++() {
      const auto last = arr.base_type::size();
      if (ind == last) return *this;
      for (++ind; ind != last && !arr.is_set(ind); ++ind);
      return *this;
    }
    /// @brief Advances to the next present entry
    /// @return The iterator's value from before the increment
    const_iterator operator++(int) {
      auto retval = *this;
      ++*this;
      return retval;
    }
    /// @brief Steps back to the previous present entry
    /// @return *this
    const_iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind));
      return *this;
    }
    /// @brief Steps back to the previous present entry
    /// @return The iterator's value from before the decrement
    const_iterator operator--(int) {
      auto retval = *this;
      --*this;
      return retval;
    }

    /// @brief Compares position and map identity
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if both refer to the same entry of the same map
    friend bool operator==(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    /// @copydoc operator==()
    friend bool operator!=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    const dense_dynamic_index_map& arr;
  };

  /// @brief Returns an iterator to the first present entry
  /// @return An iterator that skips the gaps between keys
  [[nodiscard]] const_iterator begin() const {
    return const_iterator{0, *this};
  }

  /// @copydoc begin() const
  [[nodiscard]] iterator begin() { return iterator{0, *this}; }

  /// @brief Returns an iterator past the last entry
  /// @return The end iterator
  [[nodiscard]] iterator end() { return iterator{base_type::size(), *this}; }

  /// @copydoc end()
  [[nodiscard]] const_iterator end() const {
    return const_iterator{base_type::size(), *this};
  }

  /// @brief Looks up an entry by key
  /// @param key The key to look for
  /// @return An iterator to the entry, or end() if the key is absent
  [[nodiscard]] const_iterator find(const Key& key) const {
    const auto ind = index_map(key);
    if (is_present(ind)) {
      return const_iterator{ind, *this};
    }
    return end();
  }

  /// @copydoc find(const Key&) const
  [[nodiscard]] iterator find(const Key& key) {
    const auto ind = index_map(key);
    if (is_present(ind)) {
      return iterator{ind, *this};
    }
    return end();
  }

  /// @brief Removes the entry an iterator refers to
  /// @param it The entry to remove
  /// @return An iterator to the next present entry, or end() if @p it did
  /// not refer to a present entry
  /// @note Erasing leaves a hole rather than shifting later entries down --
  /// the key is the index, so shifting would renumber every later key.
  const_iterator erase(const_iterator it) {
    if (!is_present(it.ind)) return end();

    base_type::reset(it.ind);
    return ++it;
  }
  /// @copydoc erase(const_iterator)
  iterator erase(iterator it) {
    if (!is_present(it.ind)) return end();
    base_type::reset(it.ind);
    return ++it;
  }

  /// @brief Removes the entry with the given key, if there is one
  /// @param key The key to remove
  /// @return 1 if an entry was removed, 0 if the key was absent
  size_t erase(const Key& key) {
    const auto it = find(key);
    if (it == end()) return 0;
    erase(it);
    return 1;
  }

  /// @brief Returns the value for @p key, inserting a value-initialised
  /// one if the key is absent
  /// @param key The key to look up
  /// @return A reference to the mapped value
  [[nodiscard]] Value& operator[](Key key) {
    const auto ind = index_map(key);
    if (ind < base_type::size()) {
      if (auto* v = base_type::operator[](ind)) {
        return v->second;
      }
      return base_type::emplace_at(ind, key, Value{})->second;
    } else {
      reserve(ind + 1);
      for (size_t i = base_type::size(); i < ind; ++i) {
        base_type::emplace_back(std::nullopt);
      }
      return base_type::emplace_back(key, Value{})->second;
    }
  }

  /// @brief Inserts a value constructed in place, unless the key is
  /// already present
  /// @tparam ...Args The constructor argument types
  /// @param key The key to insert at
  /// @param ...args The arguments forwarded to the value's constructor
  /// @return An iterator to the entry for @p key, and whether it was the
  /// one just inserted. An existing entry is left untouched.
  template <typename... Args>
  std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
    const auto ind = index_map(key);
    if (ind < base_type::size()) {
      if (is_present(ind)) {
        return {{ind, *this}, false};
      }
      base_type::emplace_at(ind, key, std::forward<Args>(args)...);
      return {{ind, *this}, true};
    }

    reserve(ind + 1);
    for (size_t i = base_type::size(); i < ind; ++i) {
      base_type::emplace_back(std::nullopt);
    }
    base_type::emplace_back(key, std::forward<Args>(args)...);
    return {{ind, *this}, true};
  }

  /// @brief Grows the backing storage to hold at least @p s slots
  /// @param s The number of slots to make room for
  void reserve(size_t s) { base_type::reserve(s); }

 private:
  bool is_present(size_t ind) const {
    return ind < base_type::size() && this->is_set(ind);
  }
};

/// @brief dense_dynamic_index_map specialised for size_t keys used directly
/// as indices.
///
/// Since the key *is* the index there is nothing to gain from storing it,
/// so only the values are kept. The iterators synthesise the key/value pair
/// that a map iterator is expected to yield.
/// @tparam Value The mapped type
template <typename Value>
class dense_dynamic_index_map<size_t, Value, identity>
    : protected vector_of_optional<Value> {
  using KeyToIndexMap = identity;
  using base_type = vector_of_optional<Value>;
  using Key = size_t;

 public:
  /// @brief Constructs an empty map
  /// @param init_count The number of slots to reserve up front
  dense_dynamic_index_map(size_t init_count = 0) { reserve(init_count); }

  /// @brief A map-like iterator over the engaged slots.
  ///
  /// This specialisation doesn't store the key alongside the value -- the
  /// key *is* the index -- so there is no `pair` object in the container to
  /// point at. Dereferencing therefore builds a proxy pair by value, and
  /// `operator->` returns an `arrow_proxy` that keeps that pair alive for
  /// the enclosing expression. `it->second` still works as it would on a
  /// std::map iterator.
  struct iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value&>;
    using reference = std::pair<const Key, Value&>;
    using const_reference = const std::pair<const Key, const Value&>;
    using pointer = arrow_proxy<std::pair<const Key, Value&>>;
    using const_pointer = arrow_proxy<const std::pair<const Key, const Value&>>;

    friend class dense_dynamic_index_map;

    /// @brief Constructs an iterator at or after slot @p i
    /// @param i The slot to start from. Empty slots are skipped forwards.
    /// @param map The map iterated over. It must outlive the iterator.
    iterator(size_t i, dense_dynamic_index_map& map) : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    /// @brief Builds the key/value pair for the current entry
    /// @return A pair of the key and a reference to the value. The pair
    /// itself is a temporary; the value reference is into the map.
    reference operator*() {
      return reference{ind, *arr.base_type::operator[](ind)};
    }

    /// @copydoc operator*()
    const_reference operator*() const {
      return const_reference{ind, *arr.base_type::operator[](ind)};
    }

    /// @brief Accesses the current entry's key and value
    /// @return A proxy holding the pair, so that @c it->second reaches the
    /// value in the map
    pointer operator->() { return pointer{**this}; }

    /// @copydoc operator->()
    const_pointer operator->() const { return const_pointer{**this}; }

    /// @brief Advances to the next present entry
    /// @return *this
    iterator& operator++() {
      const auto last = arr.base_type::size();
      if (ind == last) return *this;
      for (++ind; ind != last && !arr.is_set(ind); ++ind);
      return *this;
    }
    /// @brief Advances to the next present entry
    /// @return The iterator's value from before the increment
    iterator operator++(int) {
      auto retval = *this;
      ++*this;
      return retval;
    }
    /// @brief Steps back to the previous present entry
    /// @return *this
    iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind));
      return *this;
    }
    /// @brief Steps back to the previous present entry
    /// @return The iterator's value from before the decrement
    iterator operator--(int) {
      auto retval = *this;
      --*this;
      return retval;
    }

    /// @brief Compares position and map identity
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if both refer to the same entry of the same map
    [[nodiscard]] friend bool operator==(const iterator& lhs,
                                         const iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    /// @copydoc operator==()
    [[nodiscard]] friend bool operator!=(const iterator& lhs,
                                         const iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    dense_dynamic_index_map& arr;
  };

  /// @copydoc iterator
  struct const_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, const Value&>;
    using reference = const std::pair<const Key, const Value&>;
    using pointer = arrow_proxy<const std::pair<const Key, const Value&>>;

    friend class dense_dynamic_index_map;

    /// @brief Converts a mutable iterator to a const one
    /// @param it The iterator to convert
    const_iterator(iterator it) : ind(it.ind), arr(it.arr) {}
    /// @brief Constructs an iterator at or after slot @p i
    /// @param i The slot to start from. Empty slots are skipped forwards.
    /// @param map The map iterated over. It must outlive the iterator.
    const_iterator(size_t i, const dense_dynamic_index_map& map)
        : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    /// @brief Builds the key/value pair for the current entry
    /// @return A pair of the key and a reference to the value. The pair
    /// itself is a temporary; the value reference is into the map.
    reference operator*() const {
      return {ind, *arr.base_type::operator[](ind)};
    }

    /// @brief Accesses the current entry's key and value
    /// @return A proxy holding the pair, so that @c it->second reaches the
    /// value in the map
    pointer operator->() const { return pointer{**this}; }

    /// @brief Advances to the next present entry
    /// @return *this
    const_iterator& operator++() {
      const auto last = arr.base_type::size();
      if (ind == last) return *this;
      for (++ind; ind != last && !arr.is_set(ind); ++ind);
      return *this;
    }
    /// @brief Advances to the next present entry
    /// @return The iterator's value from before the increment
    const_iterator operator++(int) {
      auto retval = *this;
      ++*this;
      return retval;
    }
    /// @brief Steps back to the previous present entry
    /// @return *this
    const_iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind));
      return *this;
    }
    /// @brief Steps back to the previous present entry
    /// @return The iterator's value from before the decrement
    const_iterator operator--(int) {
      auto retval = *this;
      --*this;
      return retval;
    }

    /// @brief Compares position and map identity
    /// @param lhs The first iterator
    /// @param rhs The second iterator
    /// @return true if both refer to the same entry of the same map
    [[nodiscard]] friend bool operator==(const const_iterator& lhs,
                                         const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    /// @copydoc operator==()
    [[nodiscard]] friend bool operator!=(const const_iterator& lhs,
                                         const const_iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    const dense_dynamic_index_map& arr;
  };

  /// @brief Returns an iterator to the first present entry
  /// @return An iterator that skips the gaps between keys
  [[nodiscard]] const_iterator begin() const {
    return const_iterator{0, *this};
  }

  /// @copydoc begin() const
  [[nodiscard]] iterator begin() { return iterator{0, *this}; }

  /// @brief Returns an iterator past the last entry
  /// @return The end iterator
  [[nodiscard]] iterator end() { return iterator{base_type::size(), *this}; }

  /// @copydoc end()
  [[nodiscard]] const_iterator end() const {
    return const_iterator{base_type::size(), *this};
  }

  /// @brief Looks up an entry by key
  /// @param key The key to look for
  /// @return An iterator to the entry, or end() if the key is absent
  [[nodiscard]] const_iterator find(const Key& key) const {
    const auto ind = key;
    if (is_present(ind)) {
      return const_iterator{ind, *this};
    }
    return end();
  }

  /// @copydoc find(const Key&) const
  [[nodiscard]] iterator find(const Key& key) {
    const auto ind = key;
    if (is_present(ind)) {
      return iterator{ind, *this};
    }
    return end();
  }

  /// @brief Removes the entry an iterator refers to
  /// @param it The entry to remove
  /// @return An iterator to the next present entry, or end() if @p it did
  /// not refer to a present entry
  /// @note Erasing leaves a hole rather than shifting later entries down --
  /// the key is the index, so shifting would renumber every later key.
  const_iterator erase(const_iterator it) {
    if (!is_present(it.ind)) return end();

    base_type::reset(it.ind);
    return ++it;
  }
  /// @copydoc erase(const_iterator)
  iterator erase(iterator it) {
    if (!is_present(it.ind)) return end();
    base_type::reset(it.ind);
    return ++it;
  }

  /// @brief Removes the entry with the given key, if there is one
  /// @param key The key to remove
  /// @return 1 if an entry was removed, 0 if the key was absent
  size_t erase(const Key& key) {
    const auto it = find(key);
    if (it == end()) return 0;
    erase(it);
    return 1;
  }

  /// @brief Returns the value for @p key, inserting a value-initialised
  /// one if the key is absent
  /// @param key The key to look up
  /// @return A reference to the mapped value
  [[nodiscard]] Value& operator[](Key key) {
    const auto ind = key;
    if (ind < base_type::size()) {
      if (auto* v = base_type::operator[](ind)) {
        return *v;
      }
      return *base_type::emplace_at(ind, Value{});
    } else {
      reserve(ind + 1);
      for (size_t i = base_type::size(); i < ind; ++i) {
        base_type::emplace_back(std::nullopt);
      }
      return *base_type::emplace_back(Value{});
    }
  }

  /// @brief Inserts a value constructed in place, unless the key is
  /// already present
  /// @tparam ...Args The constructor argument types
  /// @param key The key to insert at
  /// @param ...args The arguments forwarded to the value's constructor
  /// @return An iterator to the entry for @p key, and whether it was the
  /// one just inserted. An existing entry is left untouched.
  template <typename... Args>
  std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
    const auto ind = key;
    if (ind < base_type::size()) {
      if (is_present(ind)) {
        return {{ind, *this}, false};
      }
      base_type::emplace_at(ind, std::forward<Args>(args)...);
      return {{ind, *this}, true};
    }

    reserve(ind + 1);
    for (size_t i = base_type::size(); i < ind; ++i) {
      base_type::emplace_back(std::nullopt);
    }
    base_type::emplace_back(std::forward<Args>(args)...);
    return {{ind, *this}, true};
  }

  /// @brief Grows the backing storage to hold at least @p s slots
  /// @param s The number of slots to make room for
  void reserve(size_t s) { base_type::reserve(s); }

 private:
  bool is_present(size_t ind) const {
    return ind < base_type::size() && this->is_set(ind);
  }
};

}  // namespace misc