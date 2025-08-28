#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <misc/component_table.h>

// Tests written by Chat GPT

namespace {
struct Position {
  int x = 0;
  int y = 0;
};

struct Velocity {
  int dx = 0;
  int dy = 0;
};

struct Health {
  int hp = 0;
};

constexpr size_t N = 8;
}  // namespace

class ComponentTableFixture
    : public misc::component_table<N, Position, Velocity, Health>,
      public testing::Test {
 protected:
  using base = misc::component_table<N, Position, Velocity, Health>;

  base::entity_id make_entity() {
    auto id = create_entity();
    EXPECT_TRUE(is_alive(id));
    return id;
  }
};

TEST_F(ComponentTableFixture, CreateAndDestroyEntity) {
  auto id = make_entity();
  EXPECT_TRUE(is_alive(id));

  EXPECT_EQ(destroy(id), 1);
  EXPECT_FALSE(is_alive(id));
}

TEST_F(ComponentTableFixture, GenerationInvalidatesOldIDs) {
  auto id1 = make_entity();
  auto index = id1.index();

  destroy(id1);

  auto id2 = make_entity();
  EXPECT_EQ(id2.index(), index);
  EXPECT_NE(id2.generation(), id1.generation());

  EXPECT_FALSE(is_alive(id1));
  EXPECT_TRUE(is_alive(id2));
}

TEST_F(ComponentTableFixture, EmplaceAndGetComponent) {
  auto id = make_entity();

  auto& pos = emplace<Position>(id, 1, 2);
  EXPECT_EQ(pos.x, 1);
  EXPECT_EQ(pos.y, 2);

  EXPECT_EQ(get<Position>(id).x, 1);
}

TEST_F(ComponentTableFixture, EraseComponent) {
  auto id = make_entity();
  emplace<Position>(id, 1, 2);

  EXPECT_EQ(erase_component<Position>(id), 1);
  EXPECT_EQ(get_if<Position>(id), nullptr);
}

TEST_F(ComponentTableFixture, IDAndPositionReuse) {
  auto id1 = make_entity();
  auto pos1 = id_map[id1.index()];

  destroy(id1);

  auto id2 = make_entity();
  auto pos2 = id_map[id2.index()];

  EXPECT_EQ(id1.index(), id2.index());
  EXPECT_EQ(pos1, pos2);
}

TEST_F(ComponentTableFixture, VisitEntityPointers) {
  auto id = make_entity();
  emplace<Position>(id, 1, 2);

  bool called = false;
  visit_entity<0, 1>(id, [&](Position* p, Velocity* v) {
    called = true;
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(v, nullptr);
  });

  EXPECT_TRUE(called);
}

TEST_F(ComponentTableFixture, VisitAllWithEntityID) {
  auto id1 = make_entity();
  auto id2 = make_entity();

  emplace<Position>(id1, 1, 1);
  emplace<Position>(id2, 2, 2);
  emplace<Velocity>(id2, 3, 3);

  int count = 0;
  visit_all<0>([&](entity_id id, Position& ) {
    EXPECT_TRUE(is_alive(id));
    count++;
  });

  EXPECT_EQ(count, 2);
}

TEST_F(ComponentTableFixture, VisitAnyComponent) {
  auto id1 = make_entity();
  auto id2 = make_entity();

  emplace<Position>(id1, 1, 1);
  emplace<Velocity>(id2, 2, 2);

  int count = 0;
  visit_any<0, 1>([&](entity_id, Position* p, Velocity* v) {
    EXPECT_TRUE(p || v);
    count++;
  });

  EXPECT_EQ(count, 2);
}

TEST_F(ComponentTableFixture, CompactReordersByComponentPriority) {
  auto e1 = make_entity();  // Position only
  auto e2 = make_entity();  // Position + Velocity
  auto e3 = make_entity();  // Position + Velocity + Health

  emplace<Position>(e1, 1, 1);

  emplace<Position>(e2, 2, 2);
  emplace<Velocity>(e2, 1, 1);

  emplace<Position>(e3, 3, 3);
  emplace<Velocity>(e3, 2, 2);
  emplace<Health>(e3, 100);

  [[maybe_unused]]pos_t pos1 = id_map[e1.index()];
  [[maybe_unused]]pos_t pos2 = id_map[e2.index()];
  [[maybe_unused]]pos_t pos3 = id_map[e3.index()];

  compact<0, 1, 2>();

  // After compaction, highest priority (most components) first
  EXPECT_LT(id_map[e3.index()], id_map[e2.index()]);
  EXPECT_LT(id_map[e2.index()], id_map[e1.index()]);

  // Components must still be correct
  EXPECT_EQ(get<Health>(e3).hp, 100);
  EXPECT_EQ(get<Velocity>(e2).dx, 1);
  EXPECT_EQ(get<Position>(e1).x, 1);
}

TEST_F(ComponentTableFixture, ClearResetsTable) {
  auto id = make_entity();
  emplace<Position>(id, 1, 1);

  clear();

  EXPECT_FALSE(is_alive(id));

  auto id2 = make_entity();
  EXPECT_EQ(id2.index(), 0);
}
