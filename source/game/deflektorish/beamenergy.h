#pragma once

#include "game/deflektorish/beamworld.h"

namespace Deflektorish {

struct BeamEnergyConfig {
  float maxEnergy = 100.0f;
  float regenPerSecond = 8.0f;
  float selfCrossDrainPerSecond = 2.0f;
  float backtrackDrainPerSecond = 32.0f;
  float directReturnDrainPerSecond = 35.0f;
  float maxDrainPerSecond = 90.0f;
  float selfCrossPressureRisePerSecond = 6.0f;
  float selfCrossPressureDecayPerSecond = 1.0f;
  float selfCrossTrimPixels = 16.0f;
  float backtrackAngleDegrees = 135.0f;
  float directReturnAngleDegrees = 178.0f;
};

struct BeamHazards {
  int selfCrossCount = 0;
  int backtrackCount = 0;
  int directReturnCount = 0;
  float worstBacktrackDot = 1.0f;
};

struct BeamEnergyState {
  float current = 100.0f;
  float drainPerSecond = 0.0f;
  float danger = 0.0f;
  float selfCrossPressure = 0.0f;
};

BeamHazards analyzeBeamHazards(const BeamSolveResult &result,
                               const BeamEnergyConfig &config);
float calculateBeamDrainPerSecond(const BeamHazards &hazards,
                                  const BeamEnergyConfig &config);
void updateBeamEnergy(BeamEnergyState &state, const BeamHazards &hazards,
                      const BeamEnergyConfig &config, float dt);

} // namespace Deflektorish
