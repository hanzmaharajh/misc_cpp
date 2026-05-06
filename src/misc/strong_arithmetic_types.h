#include <limits>
#include <type_traits>

namespace misc {
template <typename Derived>
struct Addable {
  friend Derived operator+(Derived a, Derived b) {
    return Derived{a.underlying_value() + b.underlying_value()};
  }
  Derived& operator+=(Derived a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() += a.underlying_value();
    return d;
  }
};

template <typename Derived>
struct Subtractable {
  friend Derived operator-(Derived a, Derived b) {
    return Derived{a.underlying_value() - b.underlying_value()};
  }
  Derived& operator-=(Derived a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() -= a.underlying_value();
    return d;
  }
};

template <typename Derived>
struct Incrementable {
  Derived& operator++() {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value()++;
    return d;
  }
  Derived operator++(int) {
    Derived& d = *static_cast<Derived*>(this);
    ++*this;
    return d;
  }
};

template <typename Derived>
struct Decrementable {
  Derived& operator--() {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value()--;
    return d;
  }
  Derived operator--(int) {
    Derived& d = *static_cast<Derived*>(this);
    ++*this;
    return d;
  }
};

template <typename Derived>
struct Multipliable {
  template <typename T>
  friend Derived operator*(T a, Derived b) {
    return Derived{a * b.underlying_value()};
  }
  template <typename T>
  friend Derived operator*(Derived b, T a) {
    return Derived{a * b.underlying_value()};
  }
  template <typename T>
  Derived& operator*=(T a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() *= a;
    return d;
  }
};

template <typename Derived>
struct Dividable {
  friend auto operator*(Derived a, Derived b) {
    return a.underlying_value() / b.underlying_value();
  }
  template <typename T>
  friend Derived operator/(Derived b, T a) {
    return Derived{b.underlying_value() / a};
  }
  template <typename T>
  Derived& operator/=(T a) {
    Derived& d = *static_cast<Derived*>(this);
    d.underlying_value() /= a;
    return d;
  }
};

template <typename Tag, typename ArithType, template <typename> typename... Ops>
class strong_arithmetic_type
    : public Ops<strong_arithmetic_type<Tag, ArithType, Ops...>>... {
 public:
  using base_type = ArithType;

  base_type _value;

  strong_arithmetic_type() = default;
  explicit strong_arithmetic_type(base_type i) : _value{i} {}

  constexpr const base_type& underlying_value() const { return _value; }
  constexpr base_type& underlying_value() { return _value; }
};

}  // namespace misc

namespace std {
template <typename Tag, typename Int, template <class> class... Ops>
class numeric_limits<misc::strong_arithmetic_type<Tag, Int, Ops...>>
    : public numeric_limits<Int> {};
}  // namespace std