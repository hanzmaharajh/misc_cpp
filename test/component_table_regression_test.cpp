#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/component_table.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace {
struct Position {
  int x = 0;
  int y = 0;
};
struct Velocity {
  int dx = 0;
  int dy = 0;
};

constexpr size_t N = 4;

class ComponentTableRegressionFixture
    : public misc::component_table<N, Position, Velocity>,
      public testing::Test {};
}  // namespace

// Regression test for: insert()'s "table full" path does
//   return entity_id{id, nullptr};
// but entity_id's constructor is (id_index_t, size_t) -- `id` is already an
// entity_id and `nullptr` isn't a size_t, so this shouldn't compile. It
// should just be `return id;` (create_entity() already returns the
// "not alive" sentinel in that case).
TEST_F(ComponentTableRegressionFixture, InsertConstructsComponents) {
  auto id = insert(Position{1, 2}, Velocity{3, 4});

  ASSERT_TRUE(is_alive(id));
  EXPECT_EQ(get<Position>(id).x, 1);
  EXPECT_EQ(get<Position>(id).y, 2);
  EXPECT_EQ(get<Velocity>(id).dx, 3);
  EXPECT_EQ(get<Velocity>(id).dy, 4);
}

TEST_F(ComponentTableRegressionFixture, InsertReturnsInvalidIdWhenFull) {
  for (size_t i = 0; i < N; ++i) {
    ASSERT_TRUE(is_alive(create_entity()));
  }

  auto id = insert(Position{});
  EXPECT_FALSE(is_alive(id));
}

// Regression test for: insert() passed the *deduced* Args to
// emplace<Args>. An lvalue argument deduces Args as `T&`, which is never
// one of the table's component types, so index_of_v<T&, Types...> failed
// its "Type is not unique in pack" static_assert.
TEST_F(ComponentTableRegressionFixture, InsertAcceptsLvalues) {
  Position p{1, 2};
  Velocity v{3, 4};

  auto id = insert(p, v);

  ASSERT_TRUE(is_alive(id));
  EXPECT_EQ(get<Position>(id).x, 1);
  EXPECT_EQ(get<Velocity>(id).dy, 4);
}

TEST_F(ComponentTableRegressionFixture, InsertAcceptsAConstLvalue) {
  const Position p{5, 6};

  auto id = insert(p);

  ASSERT_TRUE(is_alive(id));
  EXPECT_EQ(get<Position>(id).x, 5);
  EXPECT_EQ(get_if<Velocity>(id), nullptr);
}

TEST_F(ComponentTableRegressionFixture, InsertWithNoComponents) {
  auto id = insert();
  EXPECT_TRUE(is_alive(id));
  EXPECT_EQ(get_if<Position>(id), nullptr);
}

// Regression test for: the const get_pos_if<Alt>() reinterpret_cast a
// const storage slot to a *non*-const pointer, which reinterpret_cast is
// not allowed to do -- the const accessors didn't compile.
TEST_F(ComponentTableRegressionFixture, ConstAccessors) {
  auto id = insert(Position{1, 2});

  const auto& const_table = *this;

  const auto* p = const_table.get_if<Position>(id);
  ASSERT_NE(p, nullptr);
  static_assert(std::is_const_v<std::remove_pointer_t<decltype(p)>>,
                "a const table must hand out const components");
  EXPECT_EQ(p->x, 1);

  EXPECT_EQ(const_table.get<Position>(id).y, 2);
  EXPECT_EQ(const_table.get_if<Velocity>(id), nullptr);
  EXPECT_THROW([[maybe_unused]] auto& v = const_table.get<Velocity>(id),
               std::out_of_range);
}

// Regression test for: the free lists were tracked with
// std::array::iterator members -- raw pointers into the table's own
// storage. destroy() also asserted on them before checking is_alive(), so
// destroying a dead id tripped the assert instead of returning 0.
TEST_F(ComponentTableRegressionFixture, DestroyingADeadIdIsANoOp) {
  auto id = insert(Position{1, 1});
  ASSERT_EQ(destroy(id), 1u);
  EXPECT_EQ(destroy(id), 0u);

  auto never_alive = create_entity();
  ASSERT_TRUE(is_alive(never_alive));
  ASSERT_EQ(destroy(never_alive), 1u);
  EXPECT_EQ(destroy(never_alive), 0u);
}

TEST_F(ComponentTableRegressionFixture, FreeListsRecycleAfterExhaustion) {
  std::vector<entity_id> ids;
  for (size_t i = 0; i < N; ++i) {
    ids.push_back(insert(Position{static_cast<int>(i), 0}));
    ASSERT_TRUE(is_alive(ids.back()));
  }

  // Full: further inserts must fail rather than wrap the free lists round.
  EXPECT_FALSE(is_alive(insert(Position{})));
  EXPECT_FALSE(is_alive(create_entity()));

  for (auto id : ids) {
    EXPECT_EQ(destroy(id), 1u);
  }

  // Everything is free again.
  for (size_t i = 0; i < N; ++i) {
    EXPECT_TRUE(is_alive(create_entity()));
  }
}

TEST_F(ComponentTableRegressionFixture, ComponentsAreDestroyedOnEntityDestroy) {
  misc::component_table<4, std::shared_ptr<int>, int> table;
  auto kept = std::make_shared<int>(1);

  auto id = table.create_entity();
  ASSERT_TRUE(table.is_alive(id));
  table.emplace<std::shared_ptr<int>>(id, kept);
  ASSERT_EQ(kept.use_count(), 2);

  table.destroy(id);
  EXPECT_EQ(kept.use_count(), 1);
}

TEST_F(ComponentTableRegressionFixture, ClearDestroysComponents) {
  misc::component_table<4, std::shared_ptr<int>, int> table;
  auto kept = std::make_shared<int>(1);

  auto id = table.create_entity();
  table.emplace<std::shared_ptr<int>>(id, kept);
  ASSERT_EQ(kept.use_count(), 2);

  table.clear();

  EXPECT_EQ(kept.use_count(), 1);
  EXPECT_FALSE(table.is_alive(id));
}

TEST_F(ComponentTableRegressionFixture, EmplaceOnADeadIdThrows) {
  auto id = insert(Position{});
  ASSERT_EQ(destroy(id), 1u);

  EXPECT_THROW([[maybe_unused]] auto& p = emplace<Position>(id, 1, 2),
               std::out_of_range);
}

// Regression test for: bit() computed `1 << alt` on a plain int, which is
// undefined once alt reaches 31.
TEST(component_table_regression, ManyComponentTypesDoNotOverflowTheBitMask) {
  // 33 distinct component types, so the last one needs bit 32.
  using big_table = misc::component_table<
      2, std::integral_constant<int, 0>, std::integral_constant<int, 1>,
      std::integral_constant<int, 2>, std::integral_constant<int, 3>,
      std::integral_constant<int, 4>, std::integral_constant<int, 5>,
      std::integral_constant<int, 6>, std::integral_constant<int, 7>,
      std::integral_constant<int, 8>, std::integral_constant<int, 9>,
      std::integral_constant<int, 10>, std::integral_constant<int, 11>,
      std::integral_constant<int, 12>, std::integral_constant<int, 13>,
      std::integral_constant<int, 14>, std::integral_constant<int, 15>,
      std::integral_constant<int, 16>, std::integral_constant<int, 17>,
      std::integral_constant<int, 18>, std::integral_constant<int, 19>,
      std::integral_constant<int, 20>, std::integral_constant<int, 21>,
      std::integral_constant<int, 22>, std::integral_constant<int, 23>,
      std::integral_constant<int, 24>, std::integral_constant<int, 25>,
      std::integral_constant<int, 26>, std::integral_constant<int, 27>,
      std::integral_constant<int, 28>, std::integral_constant<int, 29>,
      std::integral_constant<int, 30>, std::integral_constant<int, 31>,
      std::integral_constant<int, 32>>;

  big_table table;
  auto id = table.create_entity();
  ASSERT_TRUE(table.is_alive(id));

  // Bit 32: the one that `1 << alt` on an int cannot represent.
  table.emplace<32>(id);
  EXPECT_NE(table.get_if<32>(id), nullptr);
  EXPECT_EQ(table.get_if<31>(id), nullptr);
  EXPECT_EQ(table.get_if<0>(id), nullptr);

  table.emplace<31>(id);
  EXPECT_NE(table.get_if<31>(id), nullptr);
  EXPECT_NE(table.get_if<32>(id), nullptr) << "setting bit 31 clobbered bit 32";
}

// The component storage is raw aligned storage, so the compiler-generated
// copy/move would move it byte-wise -- wrong for anything that isn't
// trivially copyable, and it used to leave the free-list iterators
// dangling into the source table.
TEST(component_table_regression, IsNeitherCopyableNorMovable) {
  using table = misc::component_table<4, Position>;
  static_assert(!std::is_copy_constructible_v<table>);
  static_assert(!std::is_move_constructible_v<table>);
  static_assert(!std::is_copy_assignable_v<table>);
  static_assert(!std::is_move_assignable_v<table>);
}

// Regression test for: compact() sorted its entries on
// (priority, old_pos) descending, with dead entities carrying
// old_pos == N. A *live* entity holding none of the compacted components
// has priority 0 too, so the old_pos tiebreak (N beats any real position)
// put every dead slot ahead of it. It was then compacted to a position
// past the end of the live range -- a position the rebuilt free list hands
// straight back out to the next entity created.
TEST_F(ComponentTableRegressionFixture, CompactKeepsLiveEntitiesPacked) {
  auto with_components = insert(Position{1, 1}, Velocity{2, 2});
  auto without_components = create_entity();  // alive, but no components
  ASSERT_TRUE(is_alive(with_components));
  ASSERT_TRUE(is_alive(without_components));

  compact<0, 1>();

  ASSERT_TRUE(is_alive(with_components));
  ASSERT_TRUE(is_alive(without_components));

  const auto live_count = 2u;
  EXPECT_LT(id_map[with_components.index()], live_count);
  EXPECT_LT(id_map[without_components.index()], live_count)
      << "a live entity was compacted past the end of the live range";

  // The components of the entity that has them must have come along.
  EXPECT_EQ(get<Position>(with_components).x, 1);
  EXPECT_EQ(get<Velocity>(with_components).dy, 2);

  // And the recycled position free list must not hand out a position that
  // is already occupied.
  auto fresh = insert(Position{9, 9});
  ASSERT_TRUE(is_alive(fresh));
  EXPECT_NE(id_map[fresh.index()], id_map[with_components.index()]);
  EXPECT_NE(id_map[fresh.index()], id_map[without_components.index()]);
  EXPECT_EQ(get<Position>(with_components).x, 1)
      << "the new entity was placed on top of an existing one";
  EXPECT_EQ(get<Position>(fresh).x, 9);
}
