#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "compact_int_array.h"
#include "pack_manipulation.h"

namespace misc {

template <size_t N, typename... Types>
class array_of_variant {
  using TupleType = std::tuple<Types...>;

  template <size_t Alt>
  using alt_type_t = typename std::tuple_element<Alt, TupleType>::type;

 protected:
  compact_int_array<sizeof...(Types), N> alternatives{};
  std::aligned_storage_t<std::max({sizeof(Types)..., alignof(Types)...}),
                         std::max({alignof(Types)...})>
      storage[N];

  template <size_t Alt>
  void destroy_at(size_t alt, size_t pos) {
    if constexpr (Alt == sizeof...(Types)) {
    } else if (Alt == alt) {
      auto* ptr = reinterpret_cast<alt_type_t<Alt>*>(&storage[pos]);
      std::destroy_at(ptr);
    } else {
      destroy_at<Alt + 1>(alt, pos);
    }
  }

  void destroy_at(size_t pos) { destroy_at<0>(alternatives.get(pos), pos); }

  void clear() {
    for (size_t i = 0; i < size(); ++i) {
      destroy_at(i);
    }
  }

  template <typename Array, size_t Alt>
  void copy_element_to_empty_arr(Array&& o, size_t pos) {
    if constexpr (Alt == sizeof...(Types)) {
    } else if (Alt == o.alternatives[pos]) {
      using alt_type = alt_type_t<Alt>;
      if constexpr (std::is_rvalue_reference_v<decltype(o)>) {
        new (&storage[pos])
            alt_type(std::move(reinterpret_cast<alt_type&>(o.storage[pos])));
      } else {
        new (&storage[pos])
            alt_type(reinterpret_cast<alt_type&>(o.storage[pos]));
      }
    } else {
      copy_element_to_empty_arr<Alt + 1>(std::forward<Array&&>(o), pos);
    }
  }

  template <typename Array>
  void copy_element_to_empty_arr(Array&& o, size_t pos) {
    copy_element_to_empty_arr<0>(std::forward<Array&&>(o), pos);
  }

  template <typename Array>
  void copy_to_empty_arr(Array&& o) {
    for (size_t i = 0; i < size(); ++i) {
      copy_element_to_empty_arr(std::forward<Array&&>(o), i);
    }
    alternatives = o.alternatives;
  }

  template <size_t Alt>
  static bool cmp_element(size_t alt, const void* lhs, const void* rhs) {
    if constexpr (Alt == sizeof...(Types)) {
    } else if (Alt == alt) {
      using alt_type = alt_type_t<Alt>;
      return *reinterpret_cast<const alt_type*>(lhs) ==
             *reinterpret_cast<const alt_type*>(rhs);
    } else {
      return cmp_element<Alt + 1>(alt, lhs, rhs);
    }
  }

  template <size_t Alt, typename Func>
  void visit(Func& func, size_t pos, size_t alt) {
    if constexpr (Alt == sizeof...(Types)) {
    } else if (Alt == alt) {
      using alt_type = alt_type_t<Alt>;
      func(pos, reinterpret_cast<alt_type&>(storage[pos]));
    } else {
      return visit<Alt + 1>(func, pos, alt);
    }
  }

  template <size_t Alt, typename Func>
  void visit(Func& func, size_t pos, size_t alt) const {
    if constexpr (Alt == sizeof...(Types)) {
    } else if (Alt == alt) {
      using alt_type = alt_type_t<Alt>;
      func(pos, reinterpret_cast<alt_type&>(storage[pos]));
    } else {
      return visit<Alt + 1>(func, pos, alt);
    }
  }

 public:
  array_of_variant() noexcept { fill<0>(alt_type_t<0>{}); }

  array_of_variant(const array_of_variant& o) noexcept { copy_to_empty_arr(o); }

  array_of_variant(array_of_variant&& o) noexcept {
    copy_to_empty_arr(std::move(o));
  }

  array_of_variant& operator=(const array_of_variant& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(o);
    }
    return *this;
  }

  array_of_variant& operator=(array_of_variant&& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(std::move(o));
    }
    return *this;
  }

  ~array_of_variant() { clear(); }

  void swap(array_of_variant& o) noexcept {
    using std::swap;
    swap(alternatives, o.alternatives);
    swap(storage, o.storage);
  }

  [[nodiscard]] friend bool operator==(const array_of_variant& lhs,
                                       const array_of_variant& rhs) {
    if (lhs.alternatives != rhs.alternatives) {
      return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i) {
      if (!cmp_element<0>(lhs.alternatives[i], lhs.storage[i], rhs.storage[i]))
        return false;
    }
    return true;
  }

  [[nodiscard]] friend bool operator!=(const array_of_variant& lhs,
                                       const array_of_variant& rhs) {
    return !(lhs == rhs);
  }

  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>>& get(size_t pos) {
    return get<index_of_v<Type, Types...>>(pos);
  }

  template <typename Type>
  [[nodiscard]] const alt_type_t<index_of_v<Type, Types...>>& get(size_t pos) const {
    return get<index_of_v<Type, Types...>>(pos);
  }

  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt>& get(size_t pos) {
    auto* ptr = get_if<Alt>(pos);
    if (ptr == nullptr) throw std::bad_variant_access();
    return *ptr;
  }

  template <size_t Alt>
  [[nodiscard]] const alt_type_t<Alt>& get(size_t pos) const {
    auto* ptr = get_if<Alt>(pos);
    if (ptr == nullptr) throw std::bad_variant_access();
    return *ptr;
  }

  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>>* get_if(size_t pos) {
    return get_if<index_of_v<Type, Types...>>(pos);
  }

  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>> const* get_if(
      size_t pos) const {
    return get_if<index_of_v<Type, Types...>>(pos);
  }

  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt> const* get_if(size_t pos) const {
    if (alternatives.get(pos) != Alt) return nullptr;
    return reinterpret_cast<const alt_type_t<Alt>*>(&storage[pos]);
  }

  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt>* get_if(size_t pos) {
    if (alternatives.get(pos) != Alt) return nullptr;
    return reinterpret_cast<alt_type_t<Alt>*>(&storage[pos]);
  }

  template <typename Type, typename... Args>
  alt_type_t<index_of_v<Type, Types...>>& emplace(size_t pos, Args&&... args) {
    return emplace<index_of_v<Type, Types...>>(pos,
                                               std::forward<Args>(args)...);
  }

  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace(size_t pos, Args&&... args) {
    destroy_at(pos);
    alternatives.set(pos, Alt);
    return *new (&storage[pos]) alt_type_t<Alt>(std::forward<Args>(args)...);
  }

  template <typename Type, typename T>
  void fill(const T& t) {
    fill<index_of_v<Type, Types...>>(t);
  }

  template <size_t Alt, typename T>
  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace<Alt>(i, t);
    }
  }

  [[nodiscard]] constexpr size_t size() const { return N; }

  template <typename Func>
  void visit(Func&& func) {
    for (size_t i = 0; i < size(); ++i) {
      visit(i, func);
    }
  }

  template <typename Func>
  void visit(Func&& func) const {
    for (size_t i = 0; i < size(); ++i) {
      visit(i, func);
    }
  }

  template <size_t Alt, typename Func>
  void visit_alternative(Func&& func) {
    for (size_t i = 0; i < size(); ++i) {
      if (alternatives.get(i) == Alt)
        func(i, reinterpret_cast<alt_type_t<Alt>&>(storage[i]));
    }
  }

  template <size_t Alt, typename Func>
  void visit_alternative(Func&& func) const {
    for (size_t i = 0; i < size(); ++i) {
      if (alternatives.get(i) == Alt)
        func(i, reinterpret_cast<alt_type_t<Alt>&>(storage[i]));
    }
  }

  template <typename Type, typename Func>
  void visit_alternative(Func&& func) {
    visit_alternative<index_of_v<Type, Types...>>(std::forward<Func>(func));
  }

  template <typename Type, typename Func>
  void visit_alternative(Func&& func) const {
    visit_alternative<index_of_v<Type, Types...>>(std::forward<Func>(func));
  }

  template <typename Func>
  void visit(size_t pos, Func&& func) {
    visit<0>(func, pos, alternatives.get(pos));
  }

  template <typename Func>
  void visit(size_t pos, Func&& func) const {
    visit<0>(func, pos, alternatives.get(pos));
  }
};

}  // namespace misc
