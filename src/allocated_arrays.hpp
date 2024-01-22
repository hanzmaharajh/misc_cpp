#pragma once
#include <new>

namespace misc {

// TODO Replace with std::span
template <typename T>
class arr_range {
  T* _start;
  T* _end;

 public:
  arr_range(T* start, T* end) : _start(start), _end(end) {}
  T* begin() { return _start; }
  T* end() { return _end; }
  const T* begin() const { return _start; }
  const T* end() const { return _end; }
  auto size() const { return _end - _start; }
  T& operator[](size_t ind) { return _start[ind]; }
  const T& operator[](size_t ind) const { return _start[ind]; }
};

/// @brief Allocates space on the heap for multiple arrays with a single allocation.
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

  /// @brief Returns the array allocated for the type indexed by Ind
  /// @tparam Ind The index
  /// @return the array
  template <size_t Ind>
  auto get() {
    const auto [offset, len] = spans[Ind];
    auto* start = reinterpret_cast<nth_type<Ind>*>(arr.get() + offset);
    return arr_range(start, start + len);
  }

 private:
  std::unique_ptr<std::byte[]> arr;
  std::array<std::pair<size_t, size_t>, sizeof...(Args)> spans;

  template <typename... Size, size_t... Ind>
  void populate_spans(std::index_sequence<Ind...>, Size... sizes) {
    (populate_span<Ind>(static_cast<size_t>(sizes)), ...);
  }

  template <size_t Ind>
  void populate_span(size_t size) {
    if constexpr (Ind == 0) {
      spans[Ind] = {0, size};
    } else {
      using prev_type = nth_type<Ind - 1>;
      using curr_type = nth_type<Ind>;
      const auto& prev_span = spans[Ind - 1];
      const auto prev_end =
          prev_span.first + sizeof(prev_type) * prev_span.second;
      auto align_padding = prev_end % alignof(curr_type);
      if (align_padding != 0)
        align_padding = alignof(curr_type) - align_padding;
      spans[Ind] = {prev_end + align_padding, size};
    }
  }

  void allocate_arr() {
    const auto last_ind = sizeof...(Args) - 1;
    const auto [last_offset, last_arr_len] = spans[last_ind];
    const auto total_size =
        last_offset + sizeof(nth_type<last_ind>) * last_arr_len;

#if __cplusplus >= 202002L
    arr = std::make_unique_for_overwrite<std::byte[]>(total_size);
#else
    arr = std::make_unique<std::byte[]>(total_size);
#endif
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

  template <typename... Size>
  explicit unique_arrays(Size... sizes) : Base(sizes...) {
    default_init_arrs(std::make_index_sequence<sizeof...(Args)>());
  }

  ~unique_arrays() { del_arrs(std::make_index_sequence<sizeof...(Args)>()); }

 private:
  template <typename... Size, size_t... Ind>
  void default_init_arrs(std::index_sequence<Ind...>) {
    (default_init_arr<Ind>(), ...);
  }

  template <size_t Ind>
  void default_init_arr() {
    using el_type = typename Base::template nth_type<Ind>;
    auto range = Base::template get<Ind>();
    auto begin = range.begin();
    auto end_created = begin;
    try {
      for (; begin != range.end(); ++begin) {
        new (begin) el_type();  // TODO Change initializer for for_overwrite
      }
    } catch (...) {
      std::destroy(begin, end_created);
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
        new (end_created++) el_type{std::move(el)};
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
