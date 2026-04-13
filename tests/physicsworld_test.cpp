#include "physicsworld.h"
#include <gtest/gtest.h>

TEST(PhysicsWorldTest, DynamicBodyRestsOnStaticFloor) {
  DL::PhysicsWorld world;
  world.setGravity({0.0f, -9.81f, 0.0f});

  const auto floor = world.createBody({
      .type = DL::PhysicsBodyType::Static,
      .shape = DL::PhysicsShapeDesc::makeBox({5.0f, 0.5f, 5.0f}),
      .position = {0.0f, -1.0f, 0.0f},
  });
  ASSERT_TRUE(floor.valid());

  const auto cube = world.createBody({
      .type = DL::PhysicsBodyType::Dynamic,
      .shape = DL::PhysicsShapeDesc::makeBox({0.5f, 0.5f, 0.5f}),
      .position = {0.0f, 4.0f, 0.0f},
  });
  ASSERT_TRUE(cube.valid());

  for (int i = 0; i < 240; ++i) {
    world.step(1.0f / 60.0f);
  }

  const auto cubeState = world.getBodyState(cube);
  EXPECT_GT(cubeState.position.y, -1.1f);
  EXPECT_LT(cubeState.position.y, 0.5f);
}

TEST(PhysicsWorldTest, RaycastHitsStaticFloor) {
  DL::PhysicsWorld world;
  const auto floor = world.createBody({
      .type = DL::PhysicsBodyType::Static,
      .shape = DL::PhysicsShapeDesc::makeBox({5.0f, 0.5f, 5.0f}),
      .position = {0.0f, -1.0f, 0.0f},
      .categoryBits = 0x0001,
  });
  ASSERT_TRUE(floor.valid());

  const auto hit = world.raycast({0.0f, 5.0f, 0.0f}, {0.0f, -5.0f, 0.0f});
  EXPECT_TRUE(hit.hasHit);
  EXPECT_TRUE(hit.body.valid());
  EXPECT_NEAR(hit.point.y, -0.5f, 0.15f);
  EXPECT_GT(hit.normal.y, 0.5f);
}

TEST(PhysicsWorldTest, CollisionMasksCanDisableFloorContact) {
  DL::PhysicsWorld world;
  world.setGravity({0.0f, -9.81f, 0.0f});

  const auto floor = world.createBody({
      .type = DL::PhysicsBodyType::Static,
      .shape = DL::PhysicsShapeDesc::makeBox({5.0f, 0.5f, 5.0f}),
      .position = {0.0f, -1.0f, 0.0f},
      .categoryBits = 0x0001,
      .maskBits = 0x0001,
  });
  ASSERT_TRUE(floor.valid());

  const auto cube = world.createBody({
      .type = DL::PhysicsBodyType::Dynamic,
      .shape = DL::PhysicsShapeDesc::makeBox({0.5f, 0.5f, 0.5f}),
      .position = {0.0f, 4.0f, 0.0f},
      .categoryBits = 0x0002,
      .maskBits = 0x0002,
  });
  ASSERT_TRUE(cube.valid());

  for (int i = 0; i < 240; ++i) {
    world.step(1.0f / 60.0f);
  }

  const auto cubeState = world.getBodyState(cube);
  EXPECT_LT(cubeState.position.y, -1.5f);
}
