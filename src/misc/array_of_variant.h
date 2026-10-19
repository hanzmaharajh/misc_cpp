#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <variant>

#include "compact_int_array.h"
#include "pack_manipulation.h"

namespace misc {

/// @brief A fixed-size array of N variants -- roughly a
/// std::array<std::variant<Types...>, N>, but with the discriminants packed
/// into a compact_int_array rather than one per element.
///
/// Every slot always holds one of the alternatives; there is no valueless
/// state. A default-constructed array holds a value-initialised
/// alternative 0 in every slot.
/// @tparam N The number of slots
/// @tparam ...Types The alternatives a slot may hold
template <size_t N, typename... Types>
class array_of_variant {
  using TupleType = std::tuple<Types...>;

  template <size_t Alt>
  using alt_type_t = typename std::tuple_element<Alt, TupleType>::type;

  // Terminates the Alt recursions below. Every slot always holds a valid
  // alternative index, so this is unreachable -- but saying so explicitly
  // (rather than falling out of the recursion doing nothing) is what lets
  // the optimiser see that every slot really does get constructed. Without
  // it GCC reports the copy/move constructors as leaving storage
  // uninitialised.
  [[noreturn]] static void invalid_alternative() {
    assert(false && "array_of_variant: alternative index out of range");
    std::abort();
  }

 public:
  /// @brief The packed array of per-slot alternative indices. Sized for
  /// sizeof...(Types) - 1, the largest index an element can hold.
  using alternatives_type = compact_int_array<sizeof...(Types) - 1, N>;

 protected:
  alternatives_type alternatives{};

  std::aligned_storage_t<std::max({sizeof(Types)..., alignof(Types)...}),
                         std::max({alignof(Types)...})>
      storage[N]{};

  template <size_t Alt>
  void destroy_at(size_t alt, size_t pos) {
    if constexpr (Alt == sizeof...(Types)) {
      invalid_alternative();
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

  template <size_t Alt, typename Array>
  void copy_element_to_empty_arr(Array&& o, size_t pos) {
    if constexpr (Alt == sizeof...(Types)) {
      invalid_alternative();
    } else if (Alt == o.alternatives.get(pos)) {
      using alt_type = alt_type_t<Alt>;
      if constexpr (std::is_rvalue_reference_v<decltype(o)>) {
        new (&storage[pos])
            alt_type(std::move(reinterpret_cast<alt_type&>(o.storage[pos])));
      } else {
        new (&storage[pos])
            alt_type(reinterpret_cast<const alt_type&>(o.storage[pos]));
      }
    } else {
      copy_element_to_empty_arr<Alt + 1>(std::forward<Array>(o), pos);
    }
  }

  template <typename Array>
  void copy_element_to_empty_arr(Array&& o, size_t pos) {
    copy_element_to_empty_arr<0>(std::forward<Array>(o), pos);
  }

  template <typename Array>
  void copy_to_empty_arr(Array&& o) {
    for (size_t i = 0; i < size(); ++i) {
      copy_element_to_empty_arr(std::forward<Array>(o), i);
    }
    alternatives = o.alternatives;
  }

  template <size_t Alt>
  static bool cmp_element(size_t alt, const void* lhs, const void* rhs) {
    if constexpr (Alt == sizeof...(Types)) {
      invalid_alternative();
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
      invalid_alternative();
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
      invalid_alternative();
    } else if (Alt == alt) {
      using alt_type = alt_type_t<Alt>;
      func(pos, reinterpret_cast<const alt_type&>(storage[pos]));
    } else {
      return visit<Alt + 1>(func, pos, alt);
    }
  }

 public:
  /// @brief Constructs an array holding a value-initialised alternative 0
  /// in every slot
  array_of_variant() noexcept {
    for (size_t i = 0; i < N; ++i) {
      alternatives.set(i, 0);
      new (&storage[i]) alt_type_t<0>{};
    }
  }

  /// @brief Copies every slot of @p o, alternative and value alike
  /// @param o The array to copy
  array_of_variant(const array_of_variant& o) noexcept { copy_to_empty_arr(o); }

  /// @brief Moves every slot of @p o
  /// @param o The array to move from. Its slots keep their alternatives and
  /// hold moved-from values.
  array_of_variant(array_of_variant&& o) noexcept {
    copy_to_empty_arr(std::move(o));
  }

  /// @brief Replaces the contents with a copy of @p o
  /// @param o The array to copy
  /// @return *this
  array_of_variant& operator=(const array_of_variant& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(o);
    }
    return *this;
  }

  /// @brief Replaces the contents with the values moved out of @p o
  /// @param o The array to move from
  /// @return *this
  array_of_variant& operator=(array_of_variant&& o) noexcept {
    if (&o != this) {
      clear();
      copy_to_empty_arr(std::move(o));
    }
    return *this;
  }

  /// @brief Destroys the active alternative of every slot
  ~array_of_variant() { clear(); }

  /// @brief Exchanges contents with @p o
  /// @param o The array to exchange with
  /// @note The elements are moved rather than the raw storage bytes being
  /// exchanged, since the alternatives need not be trivially relocatable.
  void swap(array_of_variant& o) noexcept {
    array_of_variant tmp(std::move(*this));
    *this = std::move(o);
    o = std::move(tmp);
  }

  /// @brief Exchanges the contents of @p lhs and @p rhs
  /// @param lhs The first array
  /// @param rhs The second array
  friend void swap(array_of_variant& lhs, array_of_variant& rhs) noexcept {
    lhs.swap(rhs);
  }

  /// @brief Compares the active alternative and value of every slot
  /// @param lhs The first array
  /// @param rhs The second array
  /// @return true if each slot holds the same alternative and those values
  /// compare equal
  [[nodiscard]] friend bool operator==(const array_of_variant& lhs,
                                       const array_of_variant& rhs) {
    if (lhs.alternatives != rhs.alternatives) {
      return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i) {
      if (!cmp_element<0>(lhs.alternatives.get(i), lhs.storage + i,
                          rhs.storage + i))
        return false;
    }
    return true;
  }

  /// @copydoc operator==()
  [[nodiscard]] friend bool operator!=(const array_of_variant& lhs,
                                       const array_of_variant& rhs) {
    return !(lhs == rhs);
  }

  /// @brief Returns the slot's value, which must be of alternative @p Type
  /// @tparam Type The expected alternative. It must appear exactly once in
  /// Types.
  /// @param pos The slot index. It must be less than N.
  /// @return A reference to the value
  /// @throws std::bad_variant_access if the slot holds another alternative
  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>>& get(size_t pos) {
    return get<index_of_v<Type, Types...>>(pos);
  }

  /// @copydoc get(size_t)
  template <typename Type>
  [[nodiscard]] const alt_type_t<index_of_v<Type, Types...>>& get(
      size_t pos) const {
    return get<index_of_v<Type, Types...>>(pos);
  }

  /// @brief Returns the slot's value, which must be of alternative @p Alt
  /// @tparam Alt The expected alternative index
  /// @param pos The slot index. It must be less than N.
  /// @return A reference to the value
  /// @throws std::bad_variant_access if the slot holds another alternative
  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt>& get(size_t pos) {
    auto* ptr = get_if<Alt>(pos);
    if (ptr == nullptr) throw std::bad_variant_access();
    return *ptr;
  }

  /// @copydoc get(size_t)
  template <size_t Alt>
  [[nodiscard]] const alt_type_t<Alt>& get(size_t pos) const {
    auto* ptr = get_if<Alt>(pos);
    if (ptr == nullptr) throw std::bad_variant_access();
    return *ptr;
  }

  /// @brief Returns the slot's value if it holds alternative @p Type
  /// @tparam Type The expected alternative. It must appear exactly once in
  /// Types.
  /// @param pos The slot index. It must be less than N.
  /// @return A pointer to the value, or nullptr if the slot holds another
  /// alternative
  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>>* get_if(size_t pos) {
    return get_if<index_of_v<Type, Types...>>(pos);
  }

  /// @copydoc get_if(size_t)
  template <typename Type>
  [[nodiscard]] alt_type_t<index_of_v<Type, Types...>> const* get_if(
      size_t pos) const {
    return get_if<index_of_v<Type, Types...>>(pos);
  }

  /// @brief Returns the slot's value if it holds alternative @p Alt
  /// @tparam Alt The expected alternative index
  /// @param pos The slot index. It must be less than N.
  /// @return A pointer to the value, or nullptr if the slot holds another
  /// alternative
  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt> const* get_if(size_t pos) const {
    if (alternatives.get(pos) != Alt) return nullptr;
    return reinterpret_cast<const alt_type_t<Alt>*>(&storage[pos]);
  }

  /// @copydoc get_if(size_t)
  template <size_t Alt>
  [[nodiscard]] alt_type_t<Alt>* get_if(size_t pos) {
    if (alternatives.get(pos) != Alt) return nullptr;
    return reinterpret_cast<alt_type_t<Alt>*>(&storage[pos]);
  }

  /// @brief Replaces the slot's value with a @p Type constructed in place,
  /// destroying whatever alternative was there
  /// @tparam Type The alternative to construct. It must appear exactly once
  /// in Types.
  /// @tparam ...Args The constructor argument types
  /// @param pos The slot index. It must be less than N.
  /// @param ...args The arguments forwarded to the alternative's constructor
  /// @return A reference to the new value
  template <typename Type, typename... Args>
  alt_type_t<index_of_v<Type, Types...>>& emplace(size_t pos, Args&&... args) {
    return emplace<index_of_v<Type, Types...>>(pos,
                                               std::forward<Args>(args)...);
  }

  /// @brief Replaces the slot's value with alternative @p Alt constructed
  /// in place, destroying whatever alternative was there
  /// @tparam Alt The alternative index to construct
  /// @tparam ...Args The constructor argument types
  /// @param pos The slot index. It must be less than N.
  /// @param ...args The arguments forwarded to the alternative's constructor
  /// @return A reference to the new value
  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace(size_t pos, Args&&... args) {
    destroy_at(pos);
    alternatives.set(pos, Alt);
    return *new (&storage[pos]) alt_type_t<Alt>(std::forward<Args>(args)...);
  }

  /// @brief Sets every slot to a @p Type constructed from @p t
  /// @tparam Type The alternative to construct. It must appear exactly once
  /// in Types.
  /// @tparam T The argument type
  /// @param t The value each slot's alternative is constructed from
  template <typename Type, typename T>
  void fill(const T& t) {
    fill<index_of_v<Type, Types...>>(t);
  }

  /// @brief Sets every slot to alternative @p Alt constructed from @p t
  /// @tparam Alt The alternative index to construct
  /// @tparam T The argument type
  /// @param t The value each slot's alternative is constructed from
  template <size_t Alt, typename T>
  void fill(const T& t) {
    for (size_t i = 0; i < size(); ++i) {
      emplace<Alt>(i, t);
    }
  }

  /// @brief Returns the number of slots
  /// @return N
  [[nodiscard]] constexpr size_t size() const { return N; }

  /// @brief Calls @p func for every slot with its active alternative
  /// @tparam Func The visitor type. It must be callable as
  /// func(size_t, Alt&) for every alternative.
  /// @param func The visitor, passed each slot's index and value
  template <typename Func>
  void visit(Func&& func) {
    for (size_t i = 0; i < size(); ++i) {
      visit(i, func);
    }
  }

  /// @copydoc visit(Func&&)
  template <typename Func>
  void visit(Func&& func) const {
    for (size_t i = 0; i < size(); ++i) {
      visit(i, func);
    }
  }

  /// @brief Calls @p func for each slot currently holding alternative @p Alt
  /// @tparam Alt The alternative index to visit
  /// @tparam Func The visitor type, callable as func(size_t, Alt&)
  /// @param func The visitor, passed each matching slot's index and value
  template <size_t Alt, typename Func>
  void visit_alternative(Func&& func) {
    for (size_t i = 0; i < size(); ++i) {
      if (alternatives.get(i) == Alt)
        func(i, reinterpret_cast<alt_type_t<Alt>&>(storage[i]));
    }
  }

  /// @copydoc visit_alternative(Func&&)
  template <size_t Alt, typename Func>
  void visit_alternative(Func&& func) const {
    for (size_t i = 0; i < size(); ++i) {
      if (alternatives.get(i) == Alt)
        func(i, reinterpret_cast<const alt_type_t<Alt>&>(storage[i]));
    }
  }

  /// @brief Calls @p func for each slot currently holding a @p Type
  /// @tparam Type The alternative to visit. It must appear exactly once in
  /// Types.
  /// @tparam Func The visitor type, callable as func(size_t, Type&)
  /// @param func The visitor, passed each matching slot's index and value
  template <typename Type, typename Func>
  void visit_alternative(Func&& func) {
    visit_alternative<index_of_v<Type, Types...>>(std::forward<Func>(func));
  }

  /// @copydoc visit_alternative(Func&&)
  template <typename Type, typename Func>
  void visit_alternative(Func&& func) const {
    visit_alternative<index_of_v<Type, Types...>>(std::forward<Func>(func));
  }

  /// @brief Calls @p func with the active alternative of one slot
  /// @tparam Func The visitor type. It must be callable as
  /// func(size_t, Alt&) for every alternative.
  /// @param pos The slot index. It must be less than N.
  /// @param func The visitor, passed the slot's index and value
  template <typename Func>
  void visit(size_t pos, Func&& func) {
    visit<0>(func, pos, alternatives.get(pos));
  }

  /// @copydoc visit(size_t, Func&&)
  template <typename Func>
  void visit(size_t pos, Func&& func) const {
    visit<0>(func, pos, alternatives.get(pos));
  }
};

}  // namespace misc
