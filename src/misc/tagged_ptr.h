#pragma once
#include <climits>

#include "log2.h"
#include "pack_manipulation.h"

namespace misc {
/// @brief A pointer that carries a small integer tag in the bits an
/// aligned address leaves spare.
///
/// An object aligned to Align bytes has log2(Align) always-zero low bits,
/// which this borrows to store a tag alongside the pointer at no extra
/// cost in space.
/// @tparam T The pointed-to type
/// @tparam Align The alignment the pointer is guaranteed to have. It sets
/// how many bits the tag gets.
template <typename T, size_t Align = alignof(T)>
class tagged_ptr {
  // TODO Use log2() when it is constexpr
  [[nodiscard]] static constexpr size_t num_bits(size_t v) {
    return v == 0 ? 0 : (v == 1 ? 1 : 1 + num_bits(v >> 1));
  }

 public:
  /// @brief How many bits of the pointer the tag occupies
  constexpr static const size_t tag_bit_width = num_bits(Align) - 1;

 protected:
  static_assert(tag_bit_width > 0, "alignment doesn't allow space for tag");
  uintptr_t m_ptr;
  static constexpr const uintptr_t tag_mask = (1 << tag_bit_width) - 1;
  static constexpr const uintptr_t ptr_mask = ~tag_mask;

 public:
  /// @brief Constructs a tagged pointer
  /// @param ptr The pointer, which must be aligned to Align
  /// @param tag The tag, which must fit in tag_bit_width bits
  tagged_ptr(T* ptr, size_t tag)
      : m_ptr(reinterpret_cast<uintptr_t>(ptr) | tag) {
    assert((tag & ptr_mask) == 0);
    assert((reinterpret_cast<uintptr_t>(ptr) & tag_mask) == 0);
  }

  /// @brief Returns the pointer with the tag bits cleared
  /// @return The stored pointer
  [[nodiscard]] T* get() const {
    return reinterpret_cast<T*>(m_ptr & ptr_mask);
  }

  /// @brief Returns the tag
  /// @return The stored tag
  [[nodiscard]] size_t tag() const { return m_ptr & tag_mask; }

  /// @brief Replaces the tag, leaving the pointer alone
  /// @param tag The new tag, which must fit in tag_bit_width bits
  void set_tag(size_t tag) {
    assert((tag & ptr_mask) == 0);
    m_ptr = (m_ptr & ptr_mask) | tag;
  }

  /// @brief Replaces the pointer, leaving the tag alone
  /// @param ptr The new pointer, which must be aligned to Align
  void reset(T* ptr = nullptr) {
    assert((reinterpret_cast<uintptr_t>(ptr) & tag_mask) == 0);
    m_ptr = (m_ptr & tag_mask) | reinterpret_cast<uintptr_t>(ptr);
  }

  /// @brief Accesses the pointed-to object's members
  /// @return The stored pointer
  T* operator->() const { return get(); }
};

/// @brief tagged_ptr over an array, adding indexing
/// @tparam T The element type
/// @tparam Align The alignment the pointer is guaranteed to have
template <typename T, size_t Align>
class tagged_ptr<T[], Align> : private tagged_ptr<T, Align> {
  using base_type = tagged_ptr<T, Align>;

 public:
  using base_type::get;
  using base_type::reset;
  using base_type::set_tag;
  using base_type::tag;
  using base_type::tag_bit_width;
  using base_type::tagged_ptr;
  using base_type::operator->;
  /// @brief Returns the element at @p i
  /// @param i The element index
  /// @return A reference to the element
  [[nodiscard]] T& operator[](size_t i) const { return get()[i]; }
};

/// @brief A tagged_ptr that owns what it points at, like std::unique_ptr
/// @tparam T The pointed-to type
template <typename T>
class unique_tagged_ptr : private tagged_ptr<T> {
  using base_type = tagged_ptr<T>;

 public:
  /// @brief Takes ownership of @p ptr
  /// @param ptr The pointer to own, which must be aligned to alignof(T)
  /// @param tag_value The tag to store alongside it
  unique_tagged_ptr(T* ptr, size_t tag_value) : base_type(ptr, tag_value) {}

  /// @brief Not copyable. The pointee is owned, not shared.
  unique_tagged_ptr(const unique_tagged_ptr&) = delete;

  /// @brief Takes over ownership from @p o
  /// @param o The pointer to move from. It is left empty.
  unique_tagged_ptr(unique_tagged_ptr&& o) noexcept {
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
  }

  /// @copydoc unique_tagged_ptr(const unique_tagged_ptr&)
  unique_tagged_ptr& operator=(const unique_tagged_ptr&) = delete;

  /// @brief Deletes the current pointee and takes over ownership from @p o
  /// @param o The pointer to move from. It is left empty.
  /// @return *this
  unique_tagged_ptr& operator=(unique_tagged_ptr&& o) {
    reset(nullptr);
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
    return *this;
  }

  /// @brief Deletes the pointee
  ~unique_tagged_ptr() { delete get(); }

  using base_type::get;
  using base_type::set_tag;
  using base_type::tag;
  using base_type::operator->;

  /// @brief Deletes the current pointee and takes ownership of @p ptr
  /// @param ptr The pointer to own, which must be aligned to alignof(T)
  void reset(T* ptr = nullptr) {
    delete get();
    base_type::reset(ptr);
  }
};

/// @brief A unique_tagged_ptr over an array, deleting it with delete[]
/// @tparam T The element type
template <typename T>
class unique_tagged_ptr<T[]> : private tagged_ptr<T[]> {
  using base_type = tagged_ptr<T[]>;

 public:
  /// @brief Takes ownership of the array at @p ptr
  /// @param ptr The array to own, which must be aligned to alignof(T)
  /// @param tag_value The tag to store alongside it
  unique_tagged_ptr(T* ptr, size_t tag_value) : base_type(ptr, tag_value) {}

  /// @brief Not copyable. The array is owned, not shared.
  unique_tagged_ptr(const unique_tagged_ptr&) = delete;

  /// @brief Takes over ownership from @p o
  /// @param o The pointer to move from. It is left empty.
  unique_tagged_ptr(unique_tagged_ptr&& o) noexcept {
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
  }

  /// @copydoc unique_tagged_ptr(const unique_tagged_ptr&)
  unique_tagged_ptr& operator=(const unique_tagged_ptr&) = delete;

  /// @brief Deletes the current array and takes over ownership from @p o
  /// @param o The pointer to move from. It is left empty.
  /// @return *this
  unique_tagged_ptr& operator=(unique_tagged_ptr&& o) {
    reset(nullptr);
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
    return *this;
  }

  /// @brief Deletes the array
  ~unique_tagged_ptr() { delete[] get(); }

  using base_type::get;
  using base_type::set_tag;
  using base_type::tag;
  using base_type::operator->;
  using base_type::operator[];

  /// @brief Deletes the current array and takes ownership of @p ptr
  /// @param ptr The array to own, which must be aligned to alignof(T)
  void reset(T* ptr = nullptr) {
    delete[] get();
    base_type::reset(ptr);
  }
};

/// @brief A non-owning pointer to one of several types, using the tag bits
/// to record which.
/// @tparam Align The alignment every pointee is guaranteed to have
/// @tparam ...Types The types the pointer may refer to. Each must be
/// distinct and no more strictly aligned than Align.
template <size_t Align, typename... Types>
class variant_tagged_ptr : private tagged_ptr<char, Align> {
  using base_type = tagged_ptr<char, Align>;

  template <typename Visitor, size_t Ind, size_t... Inds>
  auto visit(Visitor&& visitor,
             std::integer_sequence<size_t, Ind, Inds...>) const {
    if (Ind == index())
      return visitor(get_as<std::tuple_element_t<Ind, std::tuple<Types...>>>());
    if constexpr (sizeof...(Inds) == 0) {
      // TODO We should never get here. Should we terminate? Throw?
      return visitor(get_as<std::tuple_element_t<Ind, std::tuple<Types...>>>());
    } else
      return visit(std::forward<Visitor>(visitor),
                   std::integer_sequence<size_t, Inds...>{});
  }

 public:
  static_assert(sizeof...(Types) <= base_type::tag_mask);

  // TODO Should I use this check? It's more limiting. Not using it is more
  // dangerous, but maybe this is an advanced class that users should be aware
  // on their own?
  static_assert(((alignof(Types) <= Align) && ...));

  /// @brief Points at a @p Type, recording which alternative that is
  /// @tparam Type The pointee type. It must appear exactly once in Types.
  /// @param ptr The pointer, which must be aligned to Align
  template <typename Type>
  variant_tagged_ptr(Type* ptr)
      : base_type(reinterpret_cast<char*>(ptr), index_of_v<Type, Types...>) {}

  /// @brief Returns the pointer if it currently refers to a @p Type
  /// @tparam Type The expected type. It must appear exactly once in Types.
  /// @return The pointer, or nullptr if another alternative is stored
  template <typename Type>
  [[nodiscard]] Type* get_as() const {
    if (base_type::tag() != index_of_v<Type, Types...>) {
      return nullptr;
    }
    return reinterpret_cast<Type*>(base_type::get());
  }

  /// @brief Returns the pointer if it currently refers to alternative @p Ind
  /// @tparam Ind The expected alternative index
  /// @return The pointer, or nullptr if another alternative is stored
  template <size_t Ind>
  [[nodiscard]] auto* get_as() const {
    using Type = std::tuple_element_t<Ind, std::tuple<Types...>>;
    if (base_type::tag() != Ind) {
      return static_cast<Type*>(nullptr);
    }
    return reinterpret_cast<Type*>(base_type::get());
  }

  /// @brief Points at a different object, updating which alternative is
  /// recorded
  /// @tparam Type The new pointee type. It must appear exactly once in
  /// Types.
  /// @param ptr The new pointer, which must be aligned to Align
  template <typename Type>
  void reset(Type* ptr) {
    base_type::reset(reinterpret_cast<char*>(ptr));
    base_type::set_tag(index_of_v<Type, Types...>);
  }

  /// @brief Clears the pointer, leaving it referring to alternative 0
  void reset(std::nullptr_t) {
    base_type::reset(nullptr);
    base_type::set_tag(0);
  }

  /// @brief Returns which alternative is currently stored
  /// @return The index into Types
  [[nodiscard]] size_t index() const { return base_type::tag(); }

  /// @brief Calls @p visitor with the pointer as its current alternative
  /// @tparam Visitor The visitor type, callable with a pointer to any of
  /// Types
  /// @param visitor The visitor
  /// @return Whatever @p visitor returns
  template <typename Visitor>
  auto visit(Visitor&& visitor) const {
    return visit(std::forward<Visitor>(visitor),
                 std::make_index_sequence<sizeof...(Types)>{});
  }
};
}  // namespace misc