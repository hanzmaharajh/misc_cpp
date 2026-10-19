#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "allocated_storages.h"
#include "pack_manipulation.h"

namespace misc {

/// @brief A fixed-capacity entity-component table.
///
/// Entities are addressed by a generation-checked @c entity_id, so an id
/// held across a destroy() does not silently refer to whatever entity next
/// reuses that slot. Each component type gets its own contiguous array, and
/// an entity's components all live at the same position in those arrays.
/// The mapping from id to position is indirect, which lets compact()
/// rearrange positions without invalidating ids.
/// @tparam N The maximum number of simultaneously live entities
/// @tparam ...Types The component types. Each must be distinct.
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
  /// @brief The smallest unsigned type that can index N entities
  using id_index_t = std::conditional_t<
      N <= std::numeric_limits<uint8_t>::max(), uint8_t,
      std::conditional_t<
          N <= std::numeric_limits<uint16_t>::max(), uint16_t,
          std::conditional_t<N <= std::numeric_limits<uint32_t>::max(),
                             uint32_t, uint64_t>>>;

  /// @brief The type indexing a row of the component arrays
  using pos_t = id_index_t;

  /// @brief A handle to an entity: a slot index plus the generation that
  /// slot was on when the handle was made.
  ///
  /// Once the entity is destroyed the slot's generation moves on, so the
  /// old handle stops being alive rather than quietly addressing its
  /// successor.
  class entity_id {
    friend class component_table<N, Types...>;
    id_index_t _index;
    size_t _generation;

   public:
    /// @brief Constructs a handle to one entity slot
    /// @param ind The slot index
    /// @param gen The slot's generation
    entity_id(id_index_t ind, size_t gen) : _index{ind}, _generation{gen} {}

    /// @brief Reports whether the handle names a slot that exists at all
    /// @return true if the index is within the table
    /// @note A well-formed handle may still be stale. Use
    /// component_table::is_alive() for that.
    bool is_well_formed() const { return _index < N; }

    /// @brief Returns the slot index
    /// @return The index
    id_index_t index() const { return _index; }

    /// @brief Returns the generation the handle was made on
    /// @return The generation
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
  size_t id_free_ind;

  std::array<id_index_t, N> pos_free_list;
  size_t pos_free_ind;

  static constexpr presence_arr_t bit(size_t alt) {
    return static_cast<presence_arr_t>(uint64_t{1} << alt);
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

    return reinterpret_cast<const alt_type_t<Alt>*>(
        &std::get<Alt>(storage)[pos]);
  }

  template <size_t Alt>
  alt_type_t<Alt>* get_pos_if(pos_t pos) {
    if (!is_pos_set<Alt>(pos)) return nullptr;
    return pos_ptr<Alt>(pos);
  }

  template <size_t Alt>
  alt_type_t<Alt>* pos_ptr(pos_t pos) {
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
  /// @brief Destroys one component of the entity at a given row
  /// @tparam Alt The component index to erase
  /// @param pos The row. It must be less than N.
  /// @return 1 if a component was destroyed, 0 if there was none
  template <size_t Alt>
  size_t erase_component_pos(pos_t pos) {
    presence_arr_t& set_comps = pos_set_arr[pos];
    if (set_comps & bit(Alt)) {
      std::destroy_at(pos_ptr<Alt>(pos));
      set_comps &= static_cast<presence_arr_t>(~bit(Alt));
      return 1;
    }
    return 0;
  }

  /// @copydoc erase_component_pos(pos_t)
  template <typename Type>
  size_t erase_component_pos(pos_t pos) {
    constexpr const size_t comp_ind = index_of_v<Type, Types...>;
    return erase_component_pos<comp_ind>(pos);
  }

  /// @brief Destroys every component at a given row
  /// @param pos The row. It must be less than N.
  /// @return How many components were destroyed
  size_t erase_entity_pos(pos_t pos) {
    return (erase_component_pos<Types>(pos) + ...);
  }

  /// @brief Constructs a component in place at a given row, replacing any
  /// component of that type already there
  /// @tparam Type The component type. It must appear exactly once in Types.
  /// @tparam ...Args The constructor argument types
  /// @param pos The row. It must be less than N.
  /// @param ...args The arguments forwarded to the component's constructor
  /// @return A reference to the new component
  template <typename Type, typename... Args>
  alt_type_t<index_of_v<Type, Types...>>& emplace_pos(pos_t pos,
                                                      Args&&... args) {
    return emplace_pos<index_of_v<Type, Types...>>(pos,
                                                   std::forward<Args>(args)...);
  }

  /// @copydoc emplace_pos(pos_t, Args&&...)
  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace_pos(pos_t pos, Args&&... args) {
    erase_component_pos<Alt>(pos);
    presence_arr_t& set_comps = pos_set_arr[pos];
    set_comps |= bit(Alt);
    return *new (pos_ptr<Alt>(pos))
        alt_type_t<Alt>{std::forward<Args>(args)...};
  }

  /// @brief visit_entity() with the component indices supplied as a
  /// sequence rather than as explicit template arguments
  /// @tparam ...Alts The component indices to visit
  /// @tparam Func The visitor type
  /// @param id The entity to visit. It must be alive.
  /// @param func The visitor, passed one pointer per component
  /// @return Whatever @p func returns
  template <size_t... Alts, typename Func>
  auto visit_entity(std::integer_sequence<size_t, Alts...>, entity_id id,
                    Func&& func) {
    return visit_entity<Alts...>(id, std::forward<Func>(func));
  }

  /// @brief visit_all() with the component indices supplied as a sequence
  /// rather than as explicit template arguments
  /// @tparam ...Alts The component indices an entity must have
  /// @tparam Func The visitor type
  /// @param func The visitor, called once per matching entity
  /// @return Whatever visit_all() returns
  template <size_t... Alts, typename Func>
  auto visit_all(std::integer_sequence<size_t, Alts...>, Func&& func) {
    return visit_all<Alts...>(std::forward<Func>(func));
  }

  /// @brief Refills both free lists so every id and row is available again
  void init_free_lists() {
    std::iota(id_free_list.begin(), id_free_list.end(), 0);
    id_free_ind = 0;

    std::iota(pos_free_list.begin(), pos_free_list.end(), 0);
    pos_free_ind = 0;
  }

 public:
  /// @brief Constructs a table with no live entities
  component_table() {
    id_map.fill(N);
    init_free_lists();
  }

  /// @brief Not copyable. The component storage is raw aligned storage, so
  /// a compiler-generated copy would duplicate it byte-wise.
  component_table(const component_table&) = delete;

  /// @copydoc component_table(const component_table&)
  component_table(component_table&&) = delete;

  /// @copydoc component_table(const component_table&)
  component_table& operator=(const component_table&) = delete;

  /// @copydoc component_table(const component_table&)
  component_table& operator=(component_table&&) = delete;

  /// @brief Destroys every component of every live entity
  ~component_table() {
    for (pos_t i = 0; i < N; ++i) {
      erase_entity_pos(i);
    }
  }

  /// @brief Reports whether a handle still refers to a live entity
  /// @param id The handle to check
  /// @return true if the entity exists and has not been destroyed since the
  /// handle was made
  bool is_alive(entity_id id) const {
    return id.is_well_formed() && generations[id._index] == id.generation() &&
           id_map[id._index] < N;
  }

  /// @brief Returns an entity's component of the given type
  /// @tparam Type The component type. It must appear exactly once in Types.
  /// @param id The entity. It must be alive.
  /// @return A reference to the component
  /// @throws std::out_of_range if the entity is dead or has no such
  /// component
  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>& get(entity_id id) {
    return get<index_of_v<Type, Types...>>(id);
  }

  /// @copydoc get(entity_id)
  template <typename Type>
  const alt_type_t<index_of_v<Type, Types...>>& get(entity_id id) const {
    return get<index_of_v<Type, Types...>>(id);
  }

  /// @brief Returns an entity's component at the given index
  /// @tparam Alt The component index
  /// @param id The entity. It must be alive.
  /// @return A reference to the component
  /// @throws std::out_of_range if the entity is dead or has no such
  /// component
  template <size_t Alt>
  alt_type_t<Alt>& get(entity_id id) {
    auto* ptr = get_if<Alt>(id);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  /// @copydoc get(entity_id)
  template <size_t Alt>
  const alt_type_t<Alt>& get(entity_id id) const {
    auto* ptr = get_if<Alt>(id);
    if (ptr == nullptr) throw std::out_of_range("no component");
    return *ptr;
  }

  /// @brief Returns an entity's component of the given type, if it has one
  /// @tparam Type The component type. It must appear exactly once in Types.
  /// @param id The entity
  /// @return A pointer to the component, or nullptr if the entity is dead
  /// or has no such component
  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>>* get_if(entity_id id) {
    return get_if<index_of_v<Type, Types...>>(id);
  }

  /// @copydoc get_if(entity_id)
  template <typename Type>
  alt_type_t<index_of_v<Type, Types...>> const* get_if(entity_id id) const {
    return get_if<index_of_v<Type, Types...>>(id);
  }

  /// @brief Returns an entity's component at the given index, if it has one
  /// @tparam Alt The component index
  /// @param id The entity
  /// @return A pointer to the component, or nullptr if the entity is dead
  /// or has no such component
  template <size_t Alt>
  alt_type_t<Alt> const* get_if(entity_id id) const {
    if (!is_alive(id)) return nullptr;
    return get_pos_if<Alt>(id_map[id._index]);
  }

  /// @copydoc get_if(entity_id)
  template <size_t Alt>
  alt_type_t<Alt>* get_if(entity_id id) {
    if (!is_alive(id)) return nullptr;
    return get_pos_if<Alt>(id_map[id._index]);
  }

  /// @brief Destroys one of an entity's components, leaving the entity
  /// itself alive
  /// @tparam Alt The component index to erase
  /// @param id The entity
  /// @return 1 if a component was destroyed, 0 if the entity is dead or had
  /// no such component
  template <size_t Alt>
  size_t erase_component(entity_id id) {
    if (!is_alive(id)) return 0;
    return erase_component_pos<Alt>(id_map[id._index]);
  }

  /// @copydoc erase_component(entity_id)
  template <typename Type>
  size_t erase_component(entity_id id) {
    constexpr const size_t comp_ind = index_of_v<Type, Types...>;
    return erase_component<comp_ind>(id);
  }

  /// @brief Destroys an entity and all of its components, returning its id
  /// slot and row to the free lists
  /// @param id The entity
  /// @return 1 if an entity was destroyed, 0 if the handle was already dead
  /// @note The id slot's generation moves on, so existing handles to it
  /// stop being alive.
  size_t destroy(entity_id id) {
    if (!is_alive(id)) return 0;

    assert(id_free_ind != 0);
    assert(pos_free_ind != 0);

    const pos_t pos = id_map[id._index];

    id_map[id._index] = N;
    generations[id._index]++;

    id_free_list[--id_free_ind] = id._index;

    (erase_component_pos<Types>(pos), ...);

    pos_free_list[--pos_free_ind] = pos;
    return 1;
  }

  /// @brief Destroys every entity and resets the table to its initial state
  /// @note Unlike destroy(), this does not advance generations -- ids
  /// handed out afterwards may repeat earlier ones.
  void clear() {
    for (pos_t i = 0; i < N; ++i) {
      erase_entity_pos(i);
    }
    id_map.fill(N);
    init_free_lists();
  }

  /// @brief Constructs a component in place on an entity, replacing any
  /// component of that type it already had
  /// @tparam Alt The component type. It must appear exactly once in Types.
  /// @tparam ...Args The constructor argument types
  /// @param id The entity. It must be alive.
  /// @param ...args The arguments forwarded to the component's constructor
  /// @return A reference to the new component
  /// @throws std::out_of_range if the entity is dead
  template <typename Alt, typename... Args>
  Alt& emplace(entity_id id, Args&&... args) {
    return emplace<index_of_v<Alt, Types...>>(id, std::forward<Args>(args)...);
  }

  /// @copydoc emplace(entity_id, Args&&...)
  template <size_t Alt, typename... Args>
  alt_type_t<Alt>& emplace(entity_id id, Args&&... args) {
    if (!is_alive(id)) throw std::out_of_range("bad entity id");
    const pos_t pos = id_map[id._index];
    erase_component_pos<Alt>(pos);
    return emplace_pos<Alt>(pos, std::forward<Args>(args)...);
  }

  /// @brief Creates an entity with no components
  /// @return A handle to the new entity, or a handle that is not alive if
  /// the table is full
  [[nodiscard]] entity_id create_entity() {
    if (id_free_ind == N) return entity_id{N, 0};
    if (pos_free_ind == N) return entity_id{N, 0};
    const id_index_t ind = id_free_list[id_free_ind++];
    id_map[ind] = pos_free_list[pos_free_ind++];
    return entity_id{ind, generations[ind]};
  }

  /// @brief Creates an entity and gives it one component per argument
  /// @tparam ...Args The component types, deduced from the arguments. Each
  /// must appear exactly once in Types.
  /// @param ...args The components to move or copy in
  /// @return A handle to the new entity, or a handle that is not alive if
  /// the table is full
  template <typename... Args>
  entity_id insert(Args&&... args) {
    const entity_id id = create_entity();
    if (!is_alive(id)) return id;
    ((emplace<std::decay_t<Args>>(id, std::forward<Args>(args))), ...);
    return id;
  }

  /// @brief Creates an entity with a single component constructed in place
  /// @tparam Alt The component type. It must appear exactly once in Types.
  /// @tparam ...Args The constructor argument types
  /// @param ...args The arguments forwarded to the component's constructor
  /// @return The new entity's handle and a pointer to its component, or a
  /// not-alive handle and nullptr if the table is full
  template <typename Alt, typename... Args>
  std::tuple<entity_id, Alt*> create_and_emplace(Args&&... args) {
    return create_and_emplace<index_of_v<Alt, Types...>>(
        std::forward<Args>(args)...);
  }

  /// @copydoc create_and_emplace(Args&&...)
  template <size_t Alt, typename... Args>
  std::tuple<entity_id, alt_type_t<Alt>*> create_and_emplace(Args&&... args) {
    const entity_id id = create_entity();
    if (!is_alive(id)) return {id, nullptr};
    auto* ptr = &emplace<Alt>(id, std::forward<Args>(args)...);
    return {id, ptr};
  }

  /// @brief Calls @p func for every entity that has *all* of the given
  /// components
  /// @tparam ...Alts The component indices an entity must have. An empty
  /// pack means every component type.
  /// @tparam Func The visitor type. Called as func(entity_id, Alts&...) if
  /// that is viable, otherwise as func(Alts&...).
  /// @param func The visitor
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

  /// @brief Calls @p func once with one pointer per requested component of
  /// a single entity
  /// @tparam ...Alts The component indices to pass. An empty pack means
  /// every component type.
  /// @tparam Func The visitor type, called as func(Alts*...). A component
  /// the entity lacks is passed as nullptr.
  /// @param id The entity. It must be alive.
  /// @param func The visitor
  /// @return Whatever @p func returns
  /// @throws std::out_of_range if the entity is dead
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

  /// @brief visit_any() with the component indices supplied as a sequence
  /// rather than as explicit template arguments
  /// @tparam ...Alts The component indices to match
  /// @tparam Func The visitor type
  /// @param func The visitor, called once per matching entity
  template <size_t... Alts, typename Func>
  void visit_any(std::integer_sequence<size_t, Alts...>, Func&& func) {
    visit_any<Alts...>(std::forward<Func>(func));
  }

  /// @brief Calls @p func for every entity that has *any* of the given
  /// components
  /// @tparam ...Alts The component indices to match. An empty pack means
  /// every component type.
  /// @tparam Func The visitor type. Called as func(entity_id, Alts*...) if
  /// that is viable, otherwise as func(Alts*...). A component the entity
  /// lacks is passed as nullptr.
  /// @param func The visitor
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

  /// @brief Moves live entities to the front of the component arrays,
  /// grouping those with the same components together so that visit_all()
  /// walks them contiguously
  /// @tparam ...Alts The component indices to group by. Later indices in
  /// the pack rank ahead of earlier ones.
  /// @note Entity ids stay valid; only the rows behind them move.
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
                const bool lhs_live = lhs.old_pos < N;
                const bool rhs_live = rhs.old_pos < N;
                if (lhs_live != rhs_live) return lhs_live;

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
    pos_free_ind = id_free_ind;
  }

  /// @brief Returns the maximum number of simultaneously live entities
  /// @return N
  constexpr size_t capacity() const { return N; }
};

}  // namespace misc
