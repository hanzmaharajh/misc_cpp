#pragma once

#include <limits>
#include <type_traits>

namespace misc {
/// @brief Mixin giving a strong arithmetic type + and +=
/// @tparam Derived The strong type mixing this in
template <typename Derived>
struct Addable {
  /// @brief Adds two values of the same strong type
  /// @param a The first value
  /// @param b The second value
  /// @return Their sum, still tagged
  friend Derived operator+(Derived a, Derived b) {
    return Derived{a.underlying_value() + b.underlying_value()};
  }
  /// @brief Adds @p a to this value in place
  /// @param a The value to add
  /// @return *this, as the derived type
  Derived& operator+=(Derived a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() += a.underlying_value();
    return d;
  }
};

/// @brief Mixin giving a strong arithmetic type - and -=
/// @tparam Derived The strong type mixing this in
template <typename Derived>
struct Subtractable {
  /// @brief Subtracts two values of the same strong type
  /// @param a The value subtracted from
  /// @param b The value to subtract
  /// @return Their difference, still tagged
  friend Derived operator-(Derived a, Derived b) {
    return Derived{a.underlying_value() - b.underlying_value()};
  }
  /// @brief Subtracts @p a from this value in place
  /// @param a The value to subtract
  /// @return *this, as the derived type
  Derived& operator-=(Derived a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() -= a.underlying_value();
    return d;
  }
};

/// @brief Mixin giving a strong arithmetic type prefix and postfix ++
/// @tparam Derived The strong type mixing this in
template <typename Derived>
struct Incrementable {
  /// @brief Increments this value
  /// @return *this, as the derived type
  Derived& operator++() {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value()++;
    return d;
  }
  /// @brief Increments this value
  /// @return The value from before the increment
  Derived operator++(int) {
    Derived d = *static_cast<Derived*>(this);
    ++*this;
    return d;
  }
};

/// @brief Mixin giving a strong arithmetic type prefix and postfix --
/// @tparam Derived The strong type mixing this in
template <typename Derived>
struct Decrementable {
  /// @brief Decrements this value
  /// @return *this, as the derived type
  Derived& operator--() {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value()--;
    return d;
  }
  /// @brief Decrements this value
  /// @return The value from before the decrement
  Derived operator--(int) {
    Derived d = *static_cast<Derived*>(this);
    --*this;
    return d;
  }
};

namespace details {
// Scaling a strong type by a plain scalar keeps the tag; multiplying two
// tagged values does not (metres * metres isn't metres), so the scalar
// overloads have to exclude Derived itself. Without this the two operator*
// overloads below are also ambiguous for Derived * Derived.
template <typename Derived, typename T>
using if_scalar = std::enable_if_t<
    !std::is_same_v<Derived, std::remove_cv_t<std::remove_reference_t<T>>>,
    Derived>;
}  // namespace details

/// @brief Mixin giving a strong arithmetic type scaling by a plain scalar
/// @tparam Derived The strong type mixing this in
/// @note Multiplying two tagged values is deliberately not offered --
/// metres times metres is not metres.
template <typename Derived>
struct Multipliable {
  /// @brief Scales a tagged value by a scalar
  /// @tparam T The scalar type, which must not be Derived
  /// @param a The scalar
  /// @param b The tagged value
  /// @return The scaled value, still tagged
  template <typename T>
  friend details::if_scalar<Derived, T> operator*(T a, Derived b) {
    return Derived{a * b.underlying_value()};
  }
  /// @copydoc operator*(T, Derived)
  template <typename T>
  friend details::if_scalar<Derived, T> operator*(Derived b, T a) {
    return Derived{a * b.underlying_value()};
  }
  /// @brief Scales this value by a scalar in place
  /// @tparam T The scalar type
  /// @param a The scalar
  /// @return *this, as the derived type
  template <typename T>
  Derived& operator*=(T a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() *= a;
    return d;
  }
};

/// @brief Mixin giving a strong arithmetic type division
/// @tparam Derived The strong type mixing this in
template <typename Derived>
struct Dividable {
  /// @brief Divides two values of the same strong type
  /// @param a The dividend
  /// @param b The divisor
  /// @return Their ratio as the bare underlying type -- dividing two
  /// same-tagged values cancels the tag out
  friend auto operator/(Derived a, Derived b) {
    return a.underlying_value() / b.underlying_value();
  }
  /// @brief Divides a tagged value by a scalar
  /// @tparam T The scalar type, which must not be Derived
  /// @param b The tagged value
  /// @param a The scalar
  /// @return The divided value, still tagged
  template <typename T>
  friend details::if_scalar<Derived, T> operator/(Derived b, T a) {
    return Derived{b.underlying_value() / a};
  }
  /// @brief Divides this value by a scalar in place
  /// @tparam T The scalar type
  /// @param a The scalar
  /// @return *this, as the derived type
  template <typename T>
  Derived& operator/=(T a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() /= a;
    return d;
  }
};

/// @brief An arithmetic type that will not implicitly mix with other types
/// sharing the same underlying representation.
///
/// Two strong types built on the same ArithType but different Tags are
/// distinct and do not convert to one another. Only the operations listed
/// in Ops are available, so a type can be made addable without also being
/// multipliable.
/// @tparam Tag A type used only to tell this type apart from others
/// @tparam ArithType The underlying arithmetic type
/// @tparam ...Ops The operation mixins to inherit, e.g. misc::Addable
template <typename Tag, typename ArithType, template <typename> typename... Ops>
class strong_arithmetic_type
    : public Ops<strong_arithmetic_type<Tag, ArithType, Ops...>>... {
 public:
  /// @brief The underlying arithmetic type
  using base_type = ArithType;

  /// @brief The wrapped value
  base_type _value;

  /// @brief Constructs a value with the underlying type left uninitialised
  strong_arithmetic_type() = default;

  /// @brief Wraps an underlying value
  /// @param i The value to wrap
  explicit strong_arithmetic_type(base_type i) : _value{i} {}

  /// @brief Returns the wrapped value, dropping the tag
  /// @return A reference to the underlying value
  constexpr const base_type& underlying_value() const { return _value; }

  /// @copydoc underlying_value() const
  constexpr base_type& underlying_value() { return _value; }
};

}  // namespace misc

namespace std {
template <typename Tag, typename Int, template <class> class... Ops>
class numeric_limits<misc::strong_arithmetic_type<Tag, Int, Ops...>>
    : public numeric_limits<Int> {};
}  // namespace std