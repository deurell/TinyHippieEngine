#pragma once

#include "game/deflektorish/beamworld.h"

namespace Deflektorish {

struct BeamEnergyConfig {
  float maxEnergy = 100.0f;
  float regenPerSecond = 0.0f;
  float targetHitEnergyGain = 12.0f;
  float selfCrossDrainPerSecond = 16.0f;
  float backtrackDrainPerSecond = 84.0f;
  float directReturnDrainPerSecond = 96.0f;
  float maxDrainPerSecond = 240.0f;
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
void addBeamEnergy(BeamEnergyState &state, const BeamEnergyConfig &config,
                   float amount);

} // namespace Deflektorish
