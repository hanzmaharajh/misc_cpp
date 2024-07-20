#pragma once

#include <climits>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

#include "allocated_storages.h"

namespace misc {

namespace details {
struct octet {
  uint8_t bits;
  bool operator==(const octet& rhs) const { return bits == rhs.bits; }
  bool operator!=(const octet& rhs) const { return !(*this == rhs); }
};
}  // namespace details

template <typename T>
class VectorOfOptional {
 protected:
  size_t curr_size = 0;

  static constexpr size_t BITS_STORE_IND = 0;
  static constexpr size_t DATA_STORE_IND = 1;
  static allocated_storages<details::octet, T> make_storages(size_t capacity) {
    allocated_storages<details::octet, T> storages{
        (capacity + CHAR_BIT - 1) / CHAR_BIT, capacity};
    const auto& bits_range = storages.template get<BITS_STORE_IND>();
    std::fill(bits_range.begin(), bits_range.end(), details::octet{0x00});
    return storages;
  }

  allocated_storages<details::octet, T> storages;

  T* data() {
    return reinterpret_cast<T*>(
        storages.template get<DATA_STORE_IND>().begin());
  }

  static bool is_bit_set(size_t i, arr_range<details::octet> octs) {
    return octs[i / CHAR_BIT].bits & (1 << (i & ((1 << CHAR_BIT) - 1)));
  }

  static void set_bit(size_t i, arr_range<details::octet> bit_range) {
    bit_range[i / CHAR_BIT].bits |=
        static_cast<uint8_t>(1u << (i & ((1 << CHAR_BIT) - 1u)));
  }

  static void reset_bit(size_t i, arr_range<details::octet> bit_range) {
    bit_range[i >> uint8_t{CHAR_BIT}].bits &=
        static_cast<uint8_t>(~(1u << (i & ((1 << CHAR_BIT) - 1u))));
  }

  bool is_set(size_t i) const {
    return is_bit_set(i, storages.template get<BITS_STORE_IND>());
  }

  void set_bit(size_t i) {
    auto bit_range = storages.template get<BITS_STORE_IND>();
    set_bit(i, bit_range);
    bit_range[i / CHAR_BIT].bits |=
        static_cast<uint8_t>(1u << (i & ((1 << CHAR_BIT) - 1u)));
  }

  void reset_bit(size_t i) {
    auto bit_range = storages.template get<BITS_STORE_IND>();
    reset_bit(i, bit_range);
  }

  static size_t exp_reallocation_size(size_t s) {
    return std::max<size_t>(2 * (s - 1), 1);
  }

  void maybe_reallocate_and_copy_exact(size_t s) {
    auto tmp_storages = make_storages(s);

    this->copy_to_empty_storages(std::move(storages), tmp_storages);

    using std::swap;
    swap(storages, tmp_storages);
    destroy_arr_elements(tmp_storages);
  }

  void maybe_reallocate_and_copy(size_t s) {
    if (capacity() >= s) return;
    maybe_reallocate_and_copy_exact(exp_reallocation_size(s));
  }

  static void destroy_arr_elements(
      allocated_storages<details::octet, T>& storages) {
    const auto& data_range = storages.template get<DATA_STORE_IND>();
    const auto& bits_range = storages.template get<BITS_STORE_IND>();
    for (size_t i = 0; i < data_range.size(); ++i) {
      if (is_bit_set(i, bits_range)) {
        std::destroy_at(data_range.begin() + i);
      }
    }

    std::destroy_n(bits_range.begin(), bits_range.size());
  }

  template <typename Array1, typename Array2>
  static void copy_to_empty_storages(Array1&& from, Array2&& to) {
    auto to_data_range = to.template get<DATA_STORE_IND>();
    auto from_data_range = from.template get<DATA_STORE_IND>();

    auto to_bits_range = to.template get<BITS_STORE_IND>();
    auto from_bits_range = from.template get<BITS_STORE_IND>();
    std::copy(from_bits_range.begin(), from_bits_range.end(),
              to_bits_range.begin());

    for (size_t i = 0; i < from_data_range.size(); ++i) {
      if (is_bit_set(i, from_bits_range)) {
        if constexpr (std::is_rvalue_reference_v<Array1&&>) {
          new (to_data_range.begin() + i) T(std::move(from_data_range[i]));
        } else {
          new (to_data_range.begin() + i) T(from_data_range[i]);
        }
      }
    }
  }

 public:
  struct const_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const T*;
    using pointer = const T*;
    using reference = const T*;

    const_iterator(size_t index, const VectorOfOptional& v)
        : ind(index), arr(v) {}

    reference operator*() const { return arr[ind]; }
    pointer operator->() { return arr[ind]; }

    const_iterator& operator++() {
      ++ind;
      return *this;
    }
    const_iterator operator++(int) {
      auto retval = *this;
      ++retval;
      return retval;
    }
    const_iterator& operator--() {
      --ind;
      return *this;
    }
    const_iterator operator--(int) {
      auto retval = *this;
      --retval;
      return retval;
    }
    const_iterator operator+=(difference_type diff) {
      ind += diff;
      return *this;
    }
    const_iterator operator-=(difference_type diff) { return (*this += -diff); }

    friend const_iterator operator+(const const_iterator& lhs,
                                    const difference_type& rhs) {
      auto retval = lhs;
      retval += rhs;
      return retval;
    };

    friend const_iterator operator-(const const_iterator& lhs,
                                    const difference_type& rhs) {
      auto retval = lhs;
      retval -= rhs;
      return retval;
    };

    friend difference_type operator-(const const_iterator& lhs,
                                     const const_iterator& rhs) {
      return lhs.in - rhs.ind;
    };

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
    const VectorOfOptional& arr;
  };

  VectorOfOptional() noexcept : curr_size{0}, storages{make_storages(0)} {}

  VectorOfOptional(const VectorOfOptional& o) noexcept
      : storages{make_storages(o.size())} {
    resize(o.size());
    for (size_t i = 0; i < o.size(); ++i) {
      if (const auto* v = o[i]) {
        emplace_at(i, *v);
      } else {
        emplace_at(i, std::nullopt);
      }
    }
  };

  VectorOfOptional(VectorOfOptional&& o) noexcept : storages{make_storages(0)} {
    using std::swap;
    swap(o.storages, storages);
    swap(o.curr_size, curr_size);
  }

  VectorOfOptional& operator=(const VectorOfOptional& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(o.storages, storages);
    }
    return *this;
  }

  VectorOfOptional& operator=(VectorOfOptional&& o) noexcept = default;

  ~VectorOfOptional() { destroy_arr_elements(storages); };

  [[nodiscard]] friend bool operator==(const VectorOfOptional& lhs,
                                       const VectorOfOptional& rhs) {
    const auto& l_bits_range = lhs.storages.template get<BITS_STORE_IND>();
    const auto& r_bits_range = rhs.storages.template get<BITS_STORE_IND>();

    if (!std::equal(l_bits_range.begin(), l_bits_range.end(),
                    r_bits_range.begin(), r_bits_range.end())) {
      return false;
    }
    const auto& l_data_range = lhs.storages.template get<DATA_STORE_IND>();
    const auto& r_data_range = rhs.storages.template get<DATA_STORE_IND>();

    for (size_t i = 0; i < lhs.size(); ++i) {
      if (const auto& l_ptr = l_data_range[i]) {
        if (const auto& r_ptr = r_data_range[i];
            is_bit_set(i, l_bits_range) && !(l_ptr == r_ptr)) {
          return false;
        }
      }
    }
    return true;
  }

  [[nodiscard]] friend bool operator!=(const VectorOfOptional& lhs,
                                       const VectorOfOptional& rhs) {
    return !(lhs == rhs);
  }

  [[nodiscard]] T* operator[](size_t pos) {
    return is_set(pos) ? data() + pos : nullptr;
  }

  [[nodiscard]] const T* operator[](size_t pos) const {
    return const_cast<VectorOfOptional*>(this)->operator[](pos);
  }

  T* push_back(const T& arg) { return emplace_back(arg); }

  T* push_back(T&& arg) { return emplace_back(std::move(arg)); }

  template <typename... Args>
  T* emplace_back(Args&&... args) {
    maybe_reallocate_and_copy(size() + 1);
    return emplace_at(curr_size++, std::forward<Args>(args)...);
  }

  template <typename... Args>
  T* emplace_at(size_t pos, Args&&... args) {
    reset(pos);
    set_bit(pos);
    return new (data() + pos) T(std::forward<Args>(args)...);
  }

  T* emplace_at(size_t pos, std::nullopt_t) {
    reset(pos);
    return nullptr;
  }

  template <typename... Args>
  T* emplace(size_t pos, Args&&... args) {
    const auto s = size();
    if (s == pos) return emplace_back(std::forward<Args>(args)...);

    if (s + 1 <= capacity()) {
      // If we don't need to reallocate, we can just move the data behind the
      // new object
      for (size_t i = s; i > pos; --i) {
        if (is_set(i - 1)) {
          emplace_at(i, std::move(*(data() + i - 1)));
          reset(i - 1);
        } else {
          reset(i);
        }
      }
    } else {
      // We need to reallocate.
      // Copy the data to the new storage, with a hole for the new object
      auto tmp_storages = make_storages(exp_reallocation_size(s + 1));

      auto to_data_range = tmp_storages.template get<DATA_STORE_IND>();
      auto from_data_range = storages.template get<DATA_STORE_IND>();

      auto to_bits_range = tmp_storages.template get<BITS_STORE_IND>();
      auto from_bits_range = storages.template get<BITS_STORE_IND>();

      for (size_t i = 0; i < s; ++i) {
        const size_t to_pos = i + (i >= pos);
        if (is_bit_set(i, from_bits_range)) {
          set_bit(to_pos, to_bits_range);
          new (to_data_range.begin() + to_pos) T(std::move(from_data_range[i]));
        } else {
          reset_bit(to_pos, to_bits_range);
        }
      }

      using std::swap;
      swap(storages, tmp_storages);
      destroy_arr_elements(tmp_storages);
    }

    auto retval = emplace_at(pos, std::forward<Args>(args)...);
    ++curr_size;
    return retval;
  }

  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace_at(i, t);
    }
  }

  void fill(std::nullopt_t) { clear(); }

  void reset(size_t pos) {
    if (is_set(pos)) {
      reset_bit(pos);
      std::destroy_at(data() + pos);
    }
  }

  void resize(size_t s) {
    if (s < size()) {
      for (size_t i = s; i < size(); ++i) {
        reset(i);
      }
      curr_size = s;
    } else if (s > size()) {
      maybe_reallocate_and_copy_exact(s);
      curr_size = s;
    }
  }

  void erase(size_t pos) {
    reset(pos);
    for (size_t i = pos + 1; i < size(); ++i) {
      if (is_set(i)) {
        emplace_at(i - 1, std::move(*(data() + i)));
        reset(i);
      } else {
        reset(i - 1);
      }
    }
    reset_bit(size());
    --curr_size;
  }

  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      clear(i);
    }
    curr_size = 0;
  }

  void reserve(size_t s) {
    if (capacity() >= s) return;
    maybe_reallocate_and_copy_exact(s);
  }

  [[nodiscard]] size_t size() const { return curr_size; }
  [[nodiscard]] size_t capacity() const {
    return storages.template get<DATA_STORE_IND>().size();
  }
  const_iterator begin() const { return const_iterator(0, *this); }
  const_iterator end() const { return const_iterator(size(), *this); }
};

}  // namespace misc
