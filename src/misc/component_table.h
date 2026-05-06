#pragma once

#include <array>
#include <bitset>
#include <climits>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <tuple>
#include <type_traits>

#include "allocated_storages.h"

namespace misc {

template <size_t N, typename... Types>
class component_table {
 protected:
  const static constexpr size_t NUM_COMPONENTS = sizeof...(Types);
  const static constexpr size_t NUM_ENTITIES = N;
  using presence_arr_t = std::conditional_t<
      NUM_COMPONENTS <= sizeof(uint8_t) * CHAR_BIT, uint8_t,
      std::conditional_t<
          NUM_COMPONENTS <= sizeof(uint16_t) * CHAR_BIT, uint16_t,
          std::conditional_t<NUM_COMPONENTS <= sizeof(uint32_t) * CHAR_BIT,
                             uint32_t, uint64_t>>>;

 public:
  using id_index_t = std::conditional_t<
      N <= std::numeric_limits<uint8_t>::max(), uint8_t,
      std::conditional_t<
          N <= std::numeric_limits<uint16_t>::max(), uint16_t,
          std::conditional_t<N <= std::numeric_limits<uint32_t>::max(),
                             uint32_t, uint64_t>>>;

  using pos_t = id_index_t;

  class entity_id {
    friend class component_table<N, Types...>;
    id_index_t _index;
    size_t _generation;

   public:
   entity_id(id_index_t ind, size_t gen) :_index{ind}, _generation{gen}{}
    bool is_well_formed() const { return _index < N; }
    id_index_t index() const { return _index; }
    size_t generation() const { return _generation; }
  };

 protected:
  using TupleType = std::tuple<Types...>;

  template <size_t Alt>
  using alt_type_t = typename std::tuple_element<Alt, TupleType>::type;

  std::array<presence_arr_t, N> pos_set_arr{};
  std::array<pos_t, N> id_map;
  std::array<size_t, N> generations{};

  std::tuple<std::aligned_storage_t<sizeof(Types), alignof(Types)>[N]...>
      storage;

  std::array<id_index_t, N> id_free_list;
  typename std::array<id_index_t, N>::iterator id_free_list_it;

  std::array<id_index_t, N> pos_free_list;
  typename std::array<id_index_t, N>::iterator pos_free_list_it;

  static constexpr presence_arr_t bit(size_t alt) {
    return static_cast<presence_arr_t>(1<< alt);
  }

  template <size_t Alt>
  void swap_components(pos_t lhs, pos_t rhs) {
    auto* l_ptr = get_pos_if<Alt>(lhs);
    auto* r_ptr = get_pos_if<Alt>(rhs);
    if (l_ptr && r_ptr) {
      using std::swap;
      swap(*l_ptr, *r_ptr);
    } else if (l_ptr) {
      emplace_pos<Alt>(rhs, std::move(*l_ptr));
      erase_component_pos<Alt>(lhs);
    } else if (r_ptr) {
      emplace_pos<Alt>(lhs, std::move(*r_ptr));
      erase_component_pos<Alt>(rhs);
    }
  }

  template <size_t... Alts>
  void swap_entity_components(pos_t lhs, pos_t rhs,
                              std::index_sequence<Alts...>) {
    if (lhs == rhs) return;
    (swap_components<Alts>(lhs, rhs), ...);
  }

  template <typename Type>
  bool is_pos_set(pos_t pos) const {
    return is_pos_set<index_of_v<Type, Types...>>(pos);
  }
  template <size_t Alt>
  bool is_pos_set(pos_t pos) const {
    return pos_set_arr[pos] & bit(Alt);
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>& get_pos(pos_t pos) {
    return get_pos<index_of_v<Type, Types...>>(pos);
  }

  template <typename Type>
  const alt_type_t<index_of_v<Type, Types...>>& get_pos(pos_t pos) const {
    return get_pos<index_of_v<Type, Types...>>(pos);
  }

  template <size_t Alt>
  alt_type_t<Alt>& get_pos(pos_t pos) {
    auto* ptr = get_pos_if<Alt>(pos);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  template <size_t Alt>
  const alt_type_t<Alt>& get_pos(pos_t pos) const {
    auto* ptr = get_pos_if<Alt>(pos);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>* get_pos_if(pos_t pos) {
    return get_pos_if<index_of_v<Type, Types...>>(pos);
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>> const* get_pos_if(pos_t pos) const {
    return get_pos_if<index_of_v<Type, Types...>>(pos);
  }

  template <size_t Alt>
  alt_type_t<Alt> const* get_pos_if(pos_t pos) const {
    if (!is_pos_set<Alt>(pos)) return nullptr;
    return reinterpret_cast<alt_type_t<Alt>*>(&std::get<Alt>(storage)[pos]);
  }

  template <size_t Alt>
  alt_type_t<Alt>* get_pos_if(pos_t pos) {
    if (!is_pos_set<Alt>(pos)) return nullptr;
    return reinterpret_cast<alt_type_t<Alt>*>(&std::get<Alt>(storage)[pos]);
  }

  template <size_t Alt, size_t... Alts>
  struct bit_priority {
   private:
    constexpr static std::size_t get_priority() {
      static_assert(((Alt == Alts) + ...) == 1, "Type is not unique in pack");
      std::size_t i = 0;
      std::ignore = ((i++, Alt == Alts) || ...);
      return i - 1;
    };

   public:
    constexpr static std::size_t value = get_priority();
  };

 public:
  template <size_t Alt>
  size_t erase_component_pos(pos_t pos) {
    presence_arr_t& set_comps = pos_set_arr[pos];
    if (set_comps & bit(Alt)) {
      auto* ptr = get_pos_if<Alt>(pos);
      std::destroy_at(ptr);
      set_comps &= static_cast<presence_arr_t>(~bit(Alt));
      return 1;
    }
    return 0;
  }

  template <typename Type>
  size_t erase_component_pos(pos_t pos) {
    constexpr const size_t comp_ind = index_of_v<Type, Types...>;
    return erase_component_pos<comp_ind>(pos);
  }

  size_t erase_entity_pos(pos_t pos) {
    return (erase_component_pos<Types>(pos) + ...);
  }

  template <typename Type, typename... Args>
  alt_type_t<index_of_v<Type, Types...>>& emplace_pos(pos_t pos,
                                                      Args&&... args) {
    return emplace_pos<index_of_v<Type, Types...>>(pos,
                                                   std::forward<Args>(args)...);
  }

  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace_pos(pos_t pos, Args&&... args) {
    erase_component_pos<Alt>(pos);
    presence_arr_t& set_comps = pos_set_arr[pos];
    set_comps |= bit(Alt);
    auto& arr = std::get<Alt>(storage);
    auto* ptr = reinterpret_cast<alt_type_t<Alt>*>(&arr[pos]);
    return *new (ptr) alt_type_t<Alt>{std::forward<Args>(args)...};
  }

  template <size_t... Alts, typename Func>
  auto visit_entity(std::integer_sequence<size_t, Alts...>, entity_id id,
                    Func&& func) {
    return visit_entity<Alts...>(id, std::forward<Func>(func));
  }

  template <size_t... Alts, typename Func>
  auto visit_all(std::integer_sequence<size_t, Alts...>, Func&& func) {
    return visit_all<Alts...>(std::forward<Func>(func));
  }

  void init_free_lists() {
    std::iota(id_free_list.begin(), id_free_list.end(), 0);
    id_free_list_it = id_free_list.begin();

    std::iota(pos_free_list.begin(), pos_free_list.end(), 0);
    pos_free_list_it = pos_free_list.begin();
  }

 public:
  component_table() {
    id_map.fill(N);
    init_free_lists();
  }

  ~component_table() {
    for (pos_t i = 0; i < N; ++i) {
      erase_entity_pos(i);
    }
  }

  bool is_alive(entity_id id) const {
    return id.is_well_formed() && generations[id._index] == id.generation() &&
           id_map[id._index] < N;
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>& get(entity_id id) {
    return get<index_of_v<Type, Types...>>(id);
  }

  template <typename Type>
  const alt_type_t<index_of_v<Type, Types...>>& get(entity_id id) const {
    return get<index_of_v<Type, Types...>>(id);
  }

  template <size_t Alt>
  alt_type_t<Alt>& get(entity_id id) {
    auto* ptr = get_if<Alt>(id);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  template <size_t Alt>
  const alt_type_t<Alt>& get(entity_id id) const {
    auto* ptr = get_if<Alt>(id);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>* get_if(entity_id id) {
    return get_if<index_of_v<Type, Types...>>(id);
  }

  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>> const* get_if(entity_id id) const {
    return get_if<index_of_v<Type, Types...>>(id);
  }

  template <size_t Alt>
  alt_type_t<Alt> const* get_if(entity_id id) const {
    if (!is_alive(id)) return nullptr;
    return get_pos_if<Alt>(id_map[id._index]);
  }

  template <size_t Alt>
  alt_type_t<Alt>* get_if(entity_id id) {
    if (!is_alive(id)) return nullptr;
    return get_pos_if<Alt>(id_map[id._index]);
  }

  template <size_t Alt>
  size_t erase_component(entity_id id) {
    if (!is_alive(id)) return 0;
    return erase_component_pos<Alt>(id_map[id._index]);
  }

  template <typename Type>
  size_t erase_component(entity_id id) {
    constexpr const size_t comp_ind = index_of_v<Type, Types...>;
    return erase_component<comp_ind>(id);
  }

  size_t destroy(entity_id id) {
    assert(id_free_list_it != id_free_list.begin());
    assert(pos_free_list_it != pos_free_list.begin());

    if (!is_alive(id)) return 0;

    const pos_t pos = id_map[id._index];

    id_map[id._index] = N;
    generations[id._index]++;

    id_free_list_it--;
    *id_free_list_it = id._index;

    (erase_component_pos<Types>(pos), ...);

    pos_free_list_it--;
    *pos_free_list_it = pos;
    return 1;
  }

  void clear() {
    for (pos_t i = 0; i < N; ++i) {
      erase_entity_pos(i);
    }
    id_map.fill(N);
    init_free_lists();
  }

  template <typename Alt, typename... Args>
  Alt& emplace(entity_id id, Args&&... args) {
    return emplace<index_of_v<Alt, Types...>>(id, std::forward<Args>(args)...);
  }

  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace(entity_id id, Args&&... args) {
    if (!is_alive(id)) throw std::out_of_range("bad entity id");
    const pos_t pos = id_map[id._index];
    erase_component_pos<Alt>(pos);
    return emplace_pos<Alt>(pos, std::forward<Args>(args)...);
  }

  [[nodiscard]] entity_id create_entity() {
    if (id_free_list_it == id_free_list.end()) return entity_id{N, 0};
    if (pos_free_list_it == pos_free_list.end()) return entity_id{N, 0};
    const id_index_t ind = *id_free_list_it++;
    id_map[ind] = *pos_free_list_it++;
    return entity_id{ind, generations[ind]};
  }

  template <typename... Args>
  entity_id insert(Args&&... args) {
    const entity_id id = create_entity();
    if (!is_alive(id)) return entity_id{id, nullptr};

    ((emplace(id, std::forward<Args>(args))), ...);
    return id;
  }

  template <typename Alt, typename... Args>
  std::tuple<entity_id, Alt*> create_and_emplace(Args&&... args) {
    return create_and_emplace<index_of_v<Alt, Types...>>(
        std::forward<Args>(args)...);
  }

  template <size_t Alt, typename... Args>
  std::tuple<entity_id, alt_type_t<Alt>*> create_and_emplace(Args&&... args) {
    const entity_id id = create_entity();
    if (!is_alive(id)) return {id, nullptr};
    auto* ptr = &emplace<Alt>(id, std::forward<Args>(args)...);
    return {id, ptr};
  }

  template <size_t... Alts, typename Func>
  void visit_all(Func&& func) {
    if constexpr (sizeof...(Alts) == 0) {
      visit_all(std::make_integer_sequence<size_t, sizeof...(Types)>(),
                std::forward<Func>(func));
    } else {
      const constexpr presence_arr_t mask = (bit(Alts) | ...);

      if constexpr (std::is_invocable_v<Func, entity_id,
                                        alt_type_t<Alts>&...>) {
        for (id_index_t id = 0; id < N; ++id) {
          const pos_t pos = id_map[id];
          if (pos < N) {
            if ((pos_set_arr[pos] & mask) == mask) {
              func(entity_id{id, generations[id]}, get_pos<Alts>(pos)...);
            }
          }
        }
      } else {
        for (pos_t pos = 0; pos < N; ++pos) {
          if ((pos_set_arr[pos] & mask) == mask) {
            func(get_pos<Alts>(pos)...);
          }
        }
      }
    }
  }

  template <size_t... Alts, typename Func>
  auto visit_entity(entity_id id, Func&& func) {
    if (!is_alive(id)) throw std::out_of_range("bad entity id");
    if constexpr (sizeof...(Alts) > 0) {
      const auto& pos = id_map[id._index];
      return func((pos < N ? get_pos_if<Alts>(pos) : nullptr)...);
    } else {
      return visit_entity(
          std::make_integer_sequence<size_t, sizeof...(Types)>(), id,
          std::forward<Func>(func));
    }
  }

  template <size_t... Alts, typename Func>
  void visit_any(std::integer_sequence<size_t, Alts...>, Func&& func) {
    visit_any<Alts...>(std::forward<Func>(func));
  }

  template <size_t... Alts, typename Func>
  void visit_any(Func&& func) {
    if constexpr (sizeof...(Alts) == 0) {
      visit_any(std::make_integer_sequence<size_t, sizeof...(Types)>(),
                std::forward<Func>(func));
    } else {
      const constexpr presence_arr_t mask = (bit(Alts) | ...);

      if constexpr (std::is_invocable_v<Func, entity_id,
                                        alt_type_t<Alts>*...>) {
        for (id_index_t id = 0; id < N; ++id) {
          const pos_t pos = id_map[id];
          if (pos < N) {
            if ((pos_set_arr[pos] & mask) != 0) {
              func(entity_id{id, generations[id]}, get_pos_if<Alts>(pos)...);
            }
          }
        }
      } else {
        for (pos_t pos = 0; pos < N; ++pos) {
          if ((pos_set_arr[pos] & mask) != 0) {
            func(get_pos_if<Alts>(pos)...);
          }
        }
      }
    }
  }

  template <size_t... Alts>
  void compact() {
    struct Entry {
      pos_t old_pos;
      presence_arr_t priority;
    };
    std::array<Entry, N> entries;
    for (id_index_t id = 0; id < N; ++id) {
      presence_arr_t pos_priority = 0;

      const auto pos = id_map[id];
      if (pos < N)
        pos_priority = static_cast<presence_arr_t>(
            ((is_pos_set<Alts>(pos) << bit_priority<Alts, Alts...>::value) |
             ...));

      entries[id] = Entry{pos, pos_priority};
    }

    std::sort(entries.begin(), entries.end(),
              [](const Entry& lhs, const Entry& rhs) {
                return std::tie(lhs.priority, lhs.old_pos) >
                       std::tie(rhs.priority, rhs.old_pos);
              });

    std::array<pos_t, N> new_pos_of{};
    new_pos_of.fill(N);
    for (pos_t i = 0; i < N; ++i) {
      if (entries[i].old_pos < N) {
        new_pos_of[entries[i].old_pos] = i;
      }
    }

    for (id_index_t id = 0; id < N; ++id) {
      pos_t& pos = id_map[id];
      if (pos < N) {
        pos = new_pos_of[pos];
      }
    }

    for (pos_t i = 0; i < N; i++) {
      while (i != new_pos_of[i] && new_pos_of[i] < N) {
        using std::swap;
        const auto next = new_pos_of[i];
        swap_entity_components(i, next,
                               std::make_index_sequence<sizeof...(Types)>());
        swap(new_pos_of[i], new_pos_of[next]);
      }
    }

    std::iota(pos_free_list.begin(), pos_free_list.end(), 0);
    pos_free_list_it = pos_free_list.begin() +
                       std::distance(id_free_list.begin(), id_free_list_it);
  }

  constexpr size_t capacity() const { return N; }
};

}  // namespace misc
