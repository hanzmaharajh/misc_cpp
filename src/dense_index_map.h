#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <optional>
#include <utility>

#include "vector_of_optional.h"

namespace misc {

struct identity {
  using is_transparent = void;

  template <typename T>
  constexpr T&& operator()(T&& t) const noexcept {
    return std::forward<T>(t);
  }
};

template <typename Key, typename Value, typename KeyToIndexMap = identity>
class dense_dynamic_index_map
    : protected VectorOfOptional<std::pair<const Key, Value>> {
  KeyToIndexMap index_map;
  using base_type = VectorOfOptional<std::pair<const Key, Value>>;

 public:
  dense_dynamic_index_map(size_t init_count = 0,
                          KeyToIndexMap m = KeyToIndexMap{})
      : index_map(std::move(m)) {
    reserve(init_count);
  }

  struct iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value>;
    using pointer = std::pair<const Key, Value>*;
    using reference = std::pair<const Key, Value>&;
    using const_reference = const std::pair<const Key, Value>&;

    friend class dense_dynamic_index_map;

    iterator(size_t i, dense_dynamic_index_map& map) : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    reference operator*() { return *arr.base_type::operator[](ind); }
    const_reference operator*() const {
      return *arr.base_type::operator[](ind);
    }

    pointer operator->() { return arr.base_type::operator[](ind); }
    pointer operator->() const { return arr.base_type::operator[](ind); }

    iterator& operator++() {
      while (ind != arr.base_type::size() && !arr.is_set(++ind))
        ;
      return *this;
    }
    iterator operator++(int) {
      auto retval = *this;
      ++retval;
      return retval;
    }
    iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind))
        ;
      return *this;
    }
    iterator operator--(int) {
      auto retval = *this;
      --retval;
      return retval;
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    dense_dynamic_index_map& arr;
  };

  struct const_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value>;
    using pointer = const std::pair<const Key, Value>*;
    using reference = const std::pair<const Key, Value>&;

    friend class dense_dynamic_index_map;

    const_iterator(iterator it) : ind(it.i), arr(it.arr) {}
    const_iterator(size_t i, const dense_dynamic_index_map& map)
        : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    reference operator*() const { return *arr.base_type::operator[](ind); }
    pointer operator->() const { return arr.base_type::operator[](ind); }

    const_iterator& operator++() {
      while (ind != arr.base_type::size() && !arr.is_set(++ind))
        ;
      return *this;
    }
    const_iterator operator++(int) {
      auto retval = *this;
      ++retval;
      return retval;
    }
    const_iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind))
        ;
      return *this;
    }
    const_iterator operator--(int) {
      auto retval = *this;
      --retval;
      return retval;
    }

    friend bool operator==(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    friend bool operator!=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    const dense_dynamic_index_map& arr;
  };

  const_iterator begin() const { return const_iterator{0, *this}; }
  iterator begin() { return iterator{0, *this}; }

  iterator end() { return iterator{base_type::size(), *this}; }
  const_iterator end() const {
    return const_iterator{base_type::size(), *this};
  }

  const_iterator find(const Key& key) const {
    const auto ind = index_map(key);
    if (this->is_set(ind)) {
      return const_iterator{ind, *this};
    }
    return end();
  }

  iterator find(const Key& key) {
    const auto ind = index_map(key);
    if (this->is_set(ind)) {
      return iterator{ind, *this};
    }
    return end();
  }

  const_iterator erase(const_iterator it) {
    if (!this->is_set(it.ind)) return end();

    base_type::erase(it.ind);
    return it++;
  }
  iterator erase(iterator it) {
    if (!this->is_set(it.ind)) return end();
    base_type::erase(it.ind);
    return it++;
  }

  Value& operator[](Key key) {
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

  template <typename... Args>
  std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
    const auto ind = index_map(key);
    if (ind < base_type::size()) {
      if (this->is_set(ind)) {
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

  void reserve(size_t s) { base_type::reserve(s); }
};

template <typename Value>
class dense_dynamic_index_map<size_t, Value, identity>
    : protected VectorOfOptional<Value> {
  using KeyToIndexMap = identity;
  using base_type = VectorOfOptional<Value>;
  using Key = size_t;

 public:
  dense_dynamic_index_map(size_t init_count = 0) { reserve(init_count); }

  struct iterator {
    struct wrapper {
      std::pair<const Key, Value&> p;
      auto* operator->() { return &p; }
      const auto* operator->() const { return &p; }
    };

    struct const_wrapper {
      std::pair<const Key, const Value&> p;
      auto* operator->() { return &p; }
      const auto* operator->() const { return &p; }
    };

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, Value&>;
    using pointer = wrapper;
    using const_pointer = const_wrapper;
    using reference = std::pair<const Key, Value&>;
    using const_reference = const std::pair<const Key, const Value&>;

    friend class dense_dynamic_index_map;

    iterator(size_t i, dense_dynamic_index_map& map) : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    reference operator*() {
      return reference{ind, *arr.base_type::operator[](ind)};
    }
    const_reference operator*() const {
      return const_reference{ind, *arr.base_type::operator[](ind)};
    }

    pointer operator->() {
      return wrapper{
          std::pair<const Key, Value&>{ind, *arr.base_type::operator[](ind)}};
    }
    const_pointer operator->() const {
      return const_wrapper{std::pair<const Key, const Value&>{
          ind, *arr.base_type::operator[](ind)}};
    }

    iterator& operator++() {
      while (ind != arr.base_type::size() && !arr.is_set(++ind))
        ;
      return *this;
    }
    iterator operator++(int) {
      auto retval = *this;
      ++retval;
      return retval;
    }
    iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind))
        ;
      return *this;
    }
    iterator operator--(int) {
      auto retval = *this;
      --retval;
      return retval;
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    friend bool operator!=(const iterator& lhs, const iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    dense_dynamic_index_map& arr;
  };

  struct const_iterator {
    struct wrapper {
      std::pair<const Key, const Value&> p;
      auto* operator->() { return &p; }
      const auto* operator->() const { return &p; }
    };
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<const Key, const Value&>;
    using pointer = wrapper;
    using reference = const std::pair<const Key, const Value&>;

    friend class dense_dynamic_index_map;

    const_iterator(iterator it) : ind(it.i), arr(it.arr) {}
    const_iterator(size_t i, const dense_dynamic_index_map& map)
        : ind(i), arr(map) {
      while (ind != arr.base_type::size() && !arr.is_set(ind)) ++ind;
    }

    reference operator*() const {
      return {ind, *arr.base_type::operator[](ind)};
    }
    pointer operator->() const {
      return wrapper{std::pair{ind, arr.base_type::operator[](ind)}};
    }

    const_iterator& operator++() {
      while (ind != arr.base_type::size() && !arr.is_set(++ind))
        ;
      return *this;
    }
    const_iterator operator++(int) {
      auto retval = *this;
      ++retval;
      return retval;
    }
    const_iterator& operator--() {
      while (ind != 0 && !arr.is_set(--ind))
        ;
      return *this;
    }
    const_iterator operator--(int) {
      auto retval = *this;
      --retval;
      return retval;
    }

    friend bool operator==(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return lhs.ind == rhs.ind && &lhs.arr == &rhs.arr;
    };
    friend bool operator!=(const const_iterator& lhs,
                           const const_iterator& rhs) {
      return !(lhs == rhs);
    };

   private:
    size_t ind;
    const dense_dynamic_index_map& arr;
  };

  const_iterator begin() const { return const_iterator{0, *this}; }
  iterator begin() { return iterator{0, *this}; }

  iterator end() { return iterator{base_type::size(), *this}; }
  const_iterator end() const {
    return const_iterator{base_type::size(), *this};
  }

  const_iterator find(const Key& key) const {
    const auto ind = key;
    if (this->is_set(ind)) {
      return const_iterator{ind, *this};
    }
    return end();
  }

  iterator find(const Key& key) {
    const auto ind = key;
    if (this->is_set(ind)) {
      return iterator{ind, *this};
    }
    return end();
  }

  const_iterator erase(const_iterator it) {
    if (!this->is_set(it.ind)) return end();

    base_type::erase(it.ind);
    return it++;
  }
  iterator erase(iterator it) {
    if (!this->is_set(it.ind)) return end();
    base_type::erase(it.ind);
    return it++;
  }

  Value& operator[](Key key) {
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

  template <typename... Args>
  std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
    const auto ind = key;
    if (ind < base_type::size()) {
      if (this->is_set(ind)) {
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

  void reserve(size_t s) { base_type::reserve(s); }
};

}  // namespace misc