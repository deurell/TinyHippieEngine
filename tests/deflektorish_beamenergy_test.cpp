#include "game/deflektorish/beamenergy.h"

#include <gtest/gtest.h>

namespace {

Deflektorish::BeamSolveResult resultWithSegments(
    std::initializer_list<Deflektorish::BeamSegment> segments) {
  Deflektorish::BeamSolveResult result;
  result.segments = segments;
  return result;
}

TEST(DeflektorishBeamEnergyTest, DetectsSelfCrossingSegments) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 100.0f}, 0.0f},
      {{120.0f, 0.0f}, {160.0f, 0.0f}, 0.0f},
      {{0.0f, 100.0f}, {100.0f, 0.0f}, 0.0f},
  });

  Deflektorish::BeamEnergyConfig config;
  config.selfCrossTrimPixels = 8.0f;
  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, config);

  EXPECT_EQ(hazards.selfCrossCount, 1);
}

TEST(DeflektorishBeamEnergyTest, DetectsDefaultNinetyDegreeCrossing) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f},
      {{120.0f, 60.0f}, {160.0f, 60.0f}, 0.0f},
      {{25.0f, -50.0f}, {25.0f, 50.0f}, 0.0f},
  });

  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, {});

  EXPECT_EQ(hazards.selfCrossCount, 1);
}

TEST(DeflektorishBeamEnergyTest, IgnoresCrossingInsideTrimmedSegmentEnds) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f},
      {{120.0f, 60.0f}, {160.0f, 60.0f}, 0.0f},
      {{55.0f, -60.0f}, {55.0f, 60.0f}, 0.0f},
  });

  Deflektorish::BeamEnergyConfig config;
  config.selfCrossTrimPixels = 56.0f;
  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, config);

  EXPECT_EQ(hazards.selfCrossCount, 0);
}

TEST(DeflektorishBeamEnergyTest, DetectsBacktrackingTurns) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f},
      {{100.0f, 0.0f}, {20.0f, 60.0f}, 0.0f},
  });

  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, {});

  EXPECT_EQ(hazards.backtrackCount, 1);
  EXPECT_EQ(hazards.directReturnCount, 0);
}

TEST(DeflektorishBeamEnergyTest, AllowsSharpReflektorAndBlockerBounces) {
  const Deflektorish::BeamSolveResult reflektorResult = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f,
       Deflektorish::BeamSegmentEnd::Reflektor},
      {{100.0f, 0.0f}, {20.0f, 60.0f}, 0.0f},
  });
  const Deflektorish::BeamSolveResult blockerResult = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f,
       Deflektorish::BeamSegmentEnd::Blocker},
      {{100.0f, 0.0f}, {20.0f, 60.0f}, 0.0f},
  });

  const Deflektorish::BeamHazards reflektorHazards =
      Deflektorish::analyzeBeamHazards(reflektorResult, {});
  const Deflektorish::BeamHazards blockerHazards =
      Deflektorish::analyzeBeamHazards(blockerResult, {});

  EXPECT_EQ(reflektorHazards.backtrackCount, 0);
  EXPECT_EQ(blockerHazards.backtrackCount, 0);
  EXPECT_EQ(reflektorHazards.directReturnCount, 0);
  EXPECT_EQ(blockerHazards.directReturnCount, 0);
}

TEST(DeflektorishBeamEnergyTest, DetectsDirectReturnTurns) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f},
      {{100.0f, 0.0f}, {0.0f, 0.5f}, 0.0f},
  });

  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, {});

  EXPECT_EQ(hazards.directReturnCount, 1);
}

TEST(DeflektorishBeamEnergyTest, DirectReturnStillDrainsAfterReflector) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f,
       Deflektorish::BeamSegmentEnd::Reflektor},
      {{100.0f, 0.0f}, {0.0f, 0.5f}, 0.0f},
  });

  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, {});

  EXPECT_EQ(hazards.directReturnCount, 1);
}

TEST(DeflektorishBeamEnergyTest, NearReturnReflectorBounceIsAllowedBelowDirectThreshold) {
  const Deflektorish::BeamSolveResult result = resultWithSegments({
      {{0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f,
       Deflektorish::BeamSegmentEnd::Reflektor},
      {{100.0f, 0.0f}, {0.4f, 8.7f}, 0.0f},
  });

  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, {});

  EXPECT_EQ(hazards.backtrackCount, 0);
  EXPECT_EQ(hazards.directReturnCount, 0);
}

TEST(DeflektorishBeamEnergyTest, DrainsButDoesNotRegenerateWhileIdle) {
  Deflektorish::BeamEnergyState state{.current = 50.0f};
  Deflektorish::BeamHazards hazards;
  hazards.selfCrossCount = 1;

  Deflektorish::updateBeamEnergy(state, hazards, {}, 0.5f);
  EXPECT_FLOAT_EQ(state.current, 42.0f);
  EXPECT_FLOAT_EQ(state.selfCrossPressure, 1.0f);

  hazards.selfCrossCount = 0;
  Deflektorish::updateBeamEnergy(state, hazards, {}, 0.5f);
  EXPECT_FLOAT_EQ(state.current, 42.0f);
  EXPECT_FLOAT_EQ(state.selfCrossPressure, 0.5f);

  Deflektorish::updateBeamEnergy(state, hazards, {}, 0.5f);
  EXPECT_FLOAT_EQ(state.current, 42.0f);
  EXPECT_FLOAT_EQ(state.selfCrossPressure, 0.0f);
}

TEST(DeflektorishBeamEnergyTest, DirectReturnDrainIsSeriousButNotLethal) {
  Deflektorish::BeamEnergyState state{.current = 50.0f};
  Deflektorish::BeamHazards hazards;
  hazards.directReturnCount = 1;

  Deflektorish::updateBeamEnergy(state, hazards, {}, 0.5f);

  EXPECT_FLOAT_EQ(state.current, 2.0f);
  EXPECT_FLOAT_EQ(state.drainPerSecond, 96.0f);
}

TEST(DeflektorishBeamEnergyTest, TargetHitGainRestoresEnergyWithoutOverflow) {
  Deflektorish::BeamEnergyState state{.current = 92.0f};
  Deflektorish::BeamEnergyConfig config;

  Deflektorish::addBeamEnergy(state, config, config.targetHitEnergyGain);

  EXPECT_FLOAT_EQ(state.current, 100.0f);
}

TEST(DeflektorishBeamEnergyTest, LatchedCrawlersEscalateEnergyDrain) {
  EXPECT_FLOAT_EQ(Deflektorish::calculateLatchDrainPerSecond(0), 0.0f);
  EXPECT_FLOAT_EQ(Deflektorish::calculateLatchDrainPerSecond(1), 3.0f);
  EXPECT_FLOAT_EQ(Deflektorish::calculateLatchDrainPerSecond(2), 7.0f);
  EXPECT_FLOAT_EQ(Deflektorish::calculateLatchDrainPerSecond(3), 12.0f);
  EXPECT_FLOAT_EQ(Deflektorish::calculateLatchDrainPerSecond(8), 12.0f);
}

} // namespace
