#include "game/deflektorish/beamworld.h"

#include <gtest/gtest.h>

namespace {

TEST(DeflektorishBeamWorldTest, StraightBeamHitsTarget) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = {0.0f, 0.0f};
  world.sourceAngle = 0.0f;
  world.targets.push_back({{100.0f, 0.0f}, true});

  const Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, 4);

  ASSERT_EQ(result.segments.size(), 1u);
  EXPECT_TRUE(result.hitTargets[0]);
  EXPECT_FLOAT_EQ(result.segments[0].start.x, 0.0f);
  EXPECT_FLOAT_EQ(result.segments[0].start.y, 0.0f);
  EXPECT_NEAR(result.segments[0].end.x, 86.0f, 0.001f);
  EXPECT_NEAR(result.segments[0].end.y, 0.0f, 0.001f);
}

TEST(DeflektorishBeamWorldTest, SplitterCreatesTwoBranches) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = {0.0f, 0.0f};
  world.sourceAngle = 0.0f;
  world.splitters.push_back({{50.0f, 0.0f}, 0.0f});

  const Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, 4);

  ASSERT_EQ(result.segments.size(), 3u);
  EXPECT_TRUE(result.activeSplitters[0]);
  EXPECT_NEAR(result.segments[0].end.x, 32.0f, 0.001f);
  EXPECT_NEAR(result.segments[0].end.y, 0.0f, 0.001f);
  EXPECT_GT(result.segments[1].end.y, 0.0f);
  EXPECT_LT(result.segments[2].end.y, 0.0f);
}

TEST(DeflektorishBeamWorldTest, SplitterOrientationRotatesBothBranches) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = {0.0f, 0.0f};
  world.sourceAngle = 0.0f;
  world.splitters.push_back({{50.0f, 0.0f}, 3.1415926535f * 0.5f});

  const Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, 4);

  ASSERT_EQ(result.segments.size(), 3u);
  EXPECT_GT(result.segments[1].end.y, 0.0f);
  EXPECT_GT(result.segments[2].end.y, 0.0f);
}

TEST(DeflektorishBeamWorldTest, PassingFilterCanHitTargetInsideFilter) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = {0.0f, 0.0f};
  world.sourceAngle = 0.0f;
  world.filters.push_back({{100.0f, 0.0f}, 0.0f});
  world.targets.push_back({{100.0f, 0.0f}, true});

  const Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, 4);

  ASSERT_EQ(result.hitTargets.size(), 1u);
  ASSERT_EQ(result.passingFilters.size(), 1u);
  EXPECT_TRUE(result.passingFilters[0]);
  EXPECT_FALSE(result.blockedFilters[0]);
  EXPECT_TRUE(result.hitTargets[0]);
}

TEST(DeflektorishBeamWorldTest, IntroSourceToSinkPathIsConnected) {
  Deflektorish::BeamWorld world;
  world.sourcePosition = {140.0f, 390.0f};
  world.sourceAngle = 0.705568178f;
  world.reflektors.push_back({{275.0f, 505.0f}, 0.251116047f});
  world.reflektors.push_back({{760.0f, 405.0f}, -1.475068809f});
  world.reflektors.push_back({{220.0f, 180.0f}, -1.414971383f});
  world.reflektors.push_back({{700.0f, 140.0f}, 0.360931131f});
  world.targets.push_back({{825.0f, 270.0f}, true});

  const Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, 18);

  ASSERT_GE(result.segments.size(), 5u);
  ASSERT_EQ(result.activeReflektors.size(), 4u);
  for (bool active : result.activeReflektors) {
    EXPECT_TRUE(active);
  }
  ASSERT_EQ(result.hitTargets.size(), 1u);
  EXPECT_TRUE(result.hitTargets.front());
}

} // namespace
