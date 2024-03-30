#pragma once

#include <tuple>

namespace misc {
namespace details {

template <std::size_t N, typename ArgsTup>
decltype(auto) forward_element(ArgsTup&& args) {
  return std::forward<std::tuple_element_t<N, ArgsTup>>(std::get<N>(args));
}

template <std::size_t Start, std::size_t... Inds>
auto make_index_sequence(std::index_sequence<Inds...>) {
  return std::integer_sequence<std::size_t, (Start + Inds)...>{};
}

template <std::size_t Start, std::size_t Len>
auto make_index_sequence() {
  return details::make_index_sequence<Start>(std::make_index_sequence<Len>());
}

template <typename... Args, std::size_t... Inds>
auto take(std::index_sequence<Inds...>, std::tuple<Args...>&& args) {
  return std::make_tuple(forward_element<Inds>(std::move(args))...);
}

template <typename... Args, std::size_t... Inds>
auto repeat(std::index_sequence<Inds...>, const std::tuple<Args...>& args) {
  return std::tuple_cat(
      ((void)Inds, args)...);
}

template <typename ArgsTup, std::size_t... Inds>
auto tie(std::index_sequence<Inds...>, ArgsTup&& args) {
  return std::forward_as_tuple(
      forward_element<Inds>(std::forward<ArgsTup>(args))...);
}

}  // namespace details

/// @brief Creates an index_sequence with elements of [Start. Start + Len).
/// @tparam Start The starting index
/// @tparam Len The length of indexes
/// @return The index_sequence
template <std::size_t Start, std::size_t Len>
[[nodiscard]] auto make_index_sequence() {
  return details::make_index_sequence<Start, Len>();
}

/// @brief Takes the first N elements from the parameter pack
/// @tparam ...Args The pack type
/// @tparam N The number to take
/// @param ...args The pack arguments
/// @return A tuple containing the first N elements of the input pack
template <std::size_t N, typename... Args>
[[nodiscard]] auto take_first(Args&&... args) {
  return details::take(make_index_sequence<0, N>(),
                       std::forward_as_tuple(std::forward<Args>(args)...));
}

/// @brief Takes the specified elements from the parameter pack
/// @tparam ...Args The pack type
/// @tparam Start The index of the first element to take
/// @tparam N The number to take
/// @param ...args The pack arguments
/// @return A tuple containing N elements starting from the Start-th element
template <std::size_t Start, std::size_t N, typename... Args>
[[nodiscard]] auto take(Args&&... args) {
  return details::take(make_index_sequence<Start, N>(),
                       std::forward_as_tuple(std::forward<Args>(args)...));
}

/// @brief Takes the last N elements from the parameter pack
/// @tparam ...Args The pack type
/// @tparam N The number to take
/// @param ...args The pack arguments
/// @return A tuple containing the last N elements of the input pack
template <std::size_t N, typename... Args>
[[nodiscard]] auto take_last(Args&&... args) {
  return details::take(make_index_sequence<sizeof...(args) - N, N>(),
                       std::forward_as_tuple(std::forward<Args>(args)...));
}

/// @brief Similar to std::tie(), but allows for the selection of elements
/// @tparam ...Args The pack type
/// @tparam Start The index of the first element to take
/// @tparam N The number to take
/// @param ...args The pack arguments
/// @return A tuple containg references to N elements from the input, starting
/// with the Start-th element
template <std::size_t Start, std::size_t N, typename... Args>
[[nodiscard]] auto tie(Args&&... args) {
  return details::tie(make_index_sequence<Start, N>(),
                      std::forward_as_tuple(std::forward<Args>(args)...));
}

template <std::size_t N, typename... Args>
[[nodiscard]] auto repeat(Args&&... args) {
  return details::repeat(std::make_index_sequence<N>(),
                         std::forward_as_tuple(args...));
}

/// @brief Transforms each element of the input
/// @tparam Func The transform function type
/// @tparam ...Args The pack type
/// @param f The transform function
/// @param ...args The pack arguments
/// @return A tuple containing the result of applying the transform function to
/// each input argument
template <typename Func, typename... Args>
[[nodiscard]] auto transform_each(Func f, Args&&... args) {
  return std::make_tuple(f(std::forward<Args>(args))...);
}

template <typename Type, typename... Args>
struct index_of {
 private:
  constexpr static std::size_t get_ind() {
    static_assert((std::is_same_v<Type, Args> + ...) == 1,
                  "Type is not unique in pack");
    std::size_t i = 0;
    ((i++, std::is_same_v<Type, Args>) || ...);
    return i - 1;
  };

 public:
  constexpr static std::size_t value = get_ind();
};

template <typename Type, typename... Args>
constexpr auto index_of_v = index_of<Type, Args...>::value;

}  // namespace misc
