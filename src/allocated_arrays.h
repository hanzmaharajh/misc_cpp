#pragma once
#include <algorithm>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "always_false.h"
#include "pack_manipulation.h"

namespace misc {

template <typename T>
class arr_range {
  T* m_start;
  T* m_end;

 public:
  arr_range(T* start, T* end) noexcept : m_start(start), m_end(end) {}
  [[nodiscard]] T* begin() const { return m_start; }
  [[nodiscard]] T* end() const { return m_end; }
  [[nodiscard]] auto size() const { return m_end - m_start; }
  [[nodiscard]] const T& operator[](size_t ind) const { return m_start[ind]; }
  [[nodiscard]] T& operator[](size_t ind) { return m_start[ind]; }
};

/// @brief Allocates space on the heap for multiple arrays with a single
/// allocation.
/// @tparam ...Args The type of each array
/// @note Only space is allocated, objects are not created
template <typename... Args>
class allocated_arrays_storage {
 public:
  template <size_t N>
  using nth_type = typename std::tuple_element<N, std::tuple<Args...>>::type;

  explicit allocated_arrays_storage(std::initializer_list<Args>... lists) {
    populate_spans(std::make_index_sequence<sizeof...(Args)>(),
                   lists.size()...);

    allocate_arr();
  }

  template <typename... Size>
  explicit allocated_arrays_storage(Size... sizes) {
    static_assert(sizeof...(Size) == sizeof...(Args));

    populate_spans(std::make_index_sequence<sizeof...(Args)>(), sizes...);

    allocate_arr();
  }

  allocated_arrays_storage(const allocated_arrays_storage&) = default;
  allocated_arrays_storage(allocated_arrays_storage&&) = default;

  allocated_arrays_storage& operator=(const allocated_arrays_storage&) =
      default;
  allocated_arrays_storage& operator=(allocated_arrays_storage&&) = default;

  /// @brief Returns the array allocated for the type indexed by Ind
  /// @tparam Ind The index
  /// @return the array
  template <size_t Ind>
  [[nodiscard]] auto get() const {
    const auto [offset, len] = m_spans[Ind];
    auto* start = reinterpret_cast<nth_type<Ind>*>(m_arr.get() + offset);
    return arr_range(start, start + len);
  }

  /// @brief Returns the array allocated for the type
  /// @tparam Type The type
  /// @return the array
  template <typename Type>
  [[nodiscard]] auto get() const {
    return get<index_of_v<Type, Args...>>();
  }

 protected:
  std::unique_ptr<std::byte[]> m_arr;
  std::array<std::pair<size_t, size_t>, sizeof...(Args)> m_spans;

  template <typename... Size, size_t... Ind>
  void populate_spans(std::index_sequence<Ind...>, Size... sizes) {
    (populate_span<Ind>(static_cast<size_t>(sizes)), ...);
  }

  template <size_t Ind>
  void populate_span(size_t size) {
    if constexpr (Ind == 0) {
      m_spans[Ind] = {0, size};
    } else {
      using prev_type = nth_type<Ind - 1>;
      using curr_type = nth_type<Ind>;
      const auto& prev_span = m_spans[Ind - 1];
      const auto prev_end =
          prev_span.first + sizeof(prev_type) * prev_span.second;
      auto align_padding = prev_end % alignof(curr_type);
      if (align_padding != 0)
        align_padding = alignof(curr_type) - align_padding;
      m_spans[Ind] = {prev_end + align_padding, size};
    }
  }

  void allocate_arr() {
    const auto last_ind = sizeof...(Args) - 1;
    const auto [last_offset, last_arr_len] = m_spans[last_ind];
    const auto total_size =
        last_offset + sizeof(nth_type<last_ind>) * last_arr_len;

    m_arr.reset(new std::byte[total_size]);
  }
};

/// @brief Creates arrays of multiple object types on the heap with a single
/// allocation.
/// @tparam ...Args The type of each array
template <typename... Args>
class unique_arrays : public allocated_arrays_storage<Args...> {
  using Base = allocated_arrays_storage<Args...>;

 public:
  explicit unique_arrays(std::initializer_list<Args>... lists)
      : Base(lists...) {
    init_lists(std::make_index_sequence<sizeof...(Args)>(), lists...);
  }

  struct default_init_t {};
  struct value_init_t {};

  template <typename... Size>
  explicit unique_arrays(Size... sizes) : Base(sizes...) {
    init_arrs<value_init_t>(std::make_index_sequence<sizeof...(Args)>());
  }

  template <typename... Size>
  explicit unique_arrays(default_init_t, Size... sizes) : Base(sizes...) {
    init_arrs<default_init_t>(std::make_index_sequence<sizeof...(Args)>());
  }

  template <typename... Size>
  explicit unique_arrays(value_init_t, Size... sizes) : Base(sizes...) {
    init_arrs<value_init_t>(std::make_index_sequence<sizeof...(Args)>());
  }

  unique_arrays(const unique_arrays&) = delete;
  unique_arrays(unique_arrays&& o) noexcept
      : Base{static_cast<Base&&>(std::move(o))} {}

  unique_arrays& operator=(const unique_arrays&) = delete;
  unique_arrays& operator=(unique_arrays&& o) noexcept {
    Base::operator=(std::move(o));
    o.m_spans.fill({0, 0});
    return *this;
  };

  ~unique_arrays() { del_arrs(std::make_index_sequence<sizeof...(Args)>()); }

 private:
  template <typename Init, typename... Size, size_t... Ind>
  void init_arrs(std::index_sequence<Ind...>) {
    (init_arr<Init, Ind>(), ...);
  }

  template <typename Init, size_t Ind>
  void init_arr() {
    using el_type = typename Base::template nth_type<Ind>;
    auto range = Base::template get<Ind>();
    auto begin = range.begin();
    try {
      for (; begin != range.end(); ++begin) {
        if constexpr (std::is_same_v<Init, default_init_t>)
          new (begin) el_type;
        else if constexpr (std::is_same_v<Init, value_init_t>)
          new (begin) el_type{};
        else
          static_assert(always_false_v<Init>, "Missing init type");
      }
    } catch (...) {
      std::destroy(range.begin(), begin);
      if constexpr (Ind > 0) del_arrs(std::make_index_sequence<Ind - 1>());
      throw;
    }
  }

  template <size_t... Ind>
  void init_lists(std::index_sequence<Ind...>,
                  std::initializer_list<Args>&... list) {
    (init_list<Ind>(list), ...);
  }

  template <size_t Ind, typename T>
  void init_list(std::initializer_list<T>& list) {
    using el_type = typename Base::template nth_type<Ind>;
    auto range = Base::template get<Ind>();
    auto end_created = range.begin();
    try {
      for (auto&& el : list) {
        new (end_created) el_type{std::move(el)};
        ++end_created;
      }
    } catch (...) {
      std::destroy(range.begin(), end_created);
      if constexpr (Ind > 0) del_arrs(std::make_index_sequence<Ind - 1>());
      throw;
    }
  }

  template <size_t... Ind>
  void del_arrs(std::index_sequence<Ind...>) {
    (del_arr<Ind>(), ...);
  }

  template <size_t Ind>
  void del_arr() {
    const auto range = Base::template get<Ind>();
    std::destroy(range.begin(), range.end());
  }
};
}  // namespace misc

namespace std {
template <typename... Args>
struct tuple_size<misc::allocated_arrays_storage<Args...>>
    : std::integral_constant<size_t, sizeof...(Args)> {};

template <std::size_t I, typename... Args>
struct tuple_element<I, misc::allocated_arrays_storage<Args...>> {
  using type = misc::arr_range<std::tuple_element_t<I, std::tuple<Args...>>>;
};

}  // namespace std
