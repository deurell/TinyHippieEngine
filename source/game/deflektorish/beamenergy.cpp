#include "beamenergy.h"

#include <algorithm>
#include <cmath>

namespace Deflektorish {
namespace {

constexpr float kEpsilon = 0.001f;
constexpr float kSharedEndpointTolerance = 8.0f;

float cross(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }

float segmentLength(const BeamSegment &segment) {
  return glm::length(segment.end - segment.start);
}

glm::vec2 segmentDirection(const BeamSegment &segment) {
  const glm::vec2 delta = segment.end - segment.start;
  const float length = glm::length(delta);
  if (length <= kEpsilon) {
    return {1.0f, 0.0f};
  }
  return delta / length;
}

bool pointsNear(glm::vec2 a, glm::vec2 b, float tolerance) {
  return glm::length(a - b) <= tolerance;
}

bool shareEndpoint(const BeamSegment &a, const BeamSegment &b) {
  return pointsNear(a.start, b.start, kSharedEndpointTolerance) ||
         pointsNear(a.start, b.end, kSharedEndpointTolerance) ||
         pointsNear(a.end, b.start, kSharedEndpointTolerance) ||
         pointsNear(a.end, b.end, kSharedEndpointTolerance);
}

bool connectedForTurn(const BeamSegment &a, const BeamSegment &b) {
  return pointsNear(a.end, b.start, kSharedEndpointTolerance);
}

bool allowsSharpBounce(const BeamSegment &segment) {
  return segment.endType == BeamSegmentEnd::Reflektor ||
         segment.endType == BeamSegmentEnd::Blocker;
}

bool trimSegment(BeamSegment &segment, float trimPixels) {
  const glm::vec2 delta = segment.end - segment.start;
  const float length = glm::length(delta);
  const float trim = std::max(trimPixels, 0.0f);
  if (length <= trim * 2.0f + kEpsilon) {
    return false;
  }
  const glm::vec2 dir = delta / length;
  segment.start += dir * trim;
  segment.end -= dir * trim;
  return true;
}

bool segmentsCrossAwayFromEndpoints(const BeamSegment &a,
                                    const BeamSegment &b,
                                    float trimPixels) {
  BeamSegment trimmedA = a;
  BeamSegment trimmedB = b;
  if (!trimSegment(trimmedA, trimPixels) ||
      !trimSegment(trimmedB, trimPixels) ||
      shareEndpoint(trimmedA, trimmedB)) {
    return false;
  }

  const glm::vec2 r = trimmedA.end - trimmedA.start;
  const glm::vec2 s = trimmedB.end - trimmedB.start;
  const float denominator = cross(r, s);
  if (std::abs(denominator) <= kEpsilon) {
    return false;
  }

  const glm::vec2 delta = trimmedB.start - trimmedA.start;
  const float t = cross(delta, s) / denominator;
  const float u = cross(delta, r) / denominator;
  return t > kEpsilon && t < 1.0f - kEpsilon && u > kEpsilon &&
         u < 1.0f - kEpsilon;
}

float cosDegrees(float degrees) {
  return std::cos(degrees * 3.1415926535f / 180.0f);
}

float moveTowards(float current, float target, float maxDelta) {
  if (current < target) {
    return std::min(current + maxDelta, target);
  }
  return std::max(current - maxDelta, target);
}

} // namespace

BeamHazards analyzeBeamHazards(const BeamSolveResult &result,
                               const BeamEnergyConfig &config) {
  BeamHazards hazards;
  const auto &segments = result.segments;

  for (std::size_t i = 0; i < segments.size(); ++i) {
    for (std::size_t j = i + 1; j < segments.size(); ++j) {
      if (j == i + 1) {
        continue;
      }
      if (segmentsCrossAwayFromEndpoints(segments[i], segments[j],
                                         config.selfCrossTrimPixels)) {
        ++hazards.selfCrossCount;
      }
    }
  }

  const float backtrackDot = cosDegrees(config.backtrackAngleDegrees);
  const float directReturnDot = cosDegrees(config.directReturnAngleDegrees);
  for (std::size_t i = 1; i < segments.size(); ++i) {
    const BeamSegment &previous = segments[i - 1];
    const BeamSegment &current = segments[i];
    if (!connectedForTurn(previous, current) ||
        segmentLength(previous) <= kEpsilon ||
        segmentLength(current) <= kEpsilon) {
      continue;
    }

    const float dot =
        glm::dot(segmentDirection(previous), segmentDirection(current));
    hazards.worstBacktrackDot = std::min(hazards.worstBacktrackDot, dot);
    if (dot <= directReturnDot) {
      ++hazards.directReturnCount;
    } else if (dot <= backtrackDot && !allowsSharpBounce(previous)) {
      ++hazards.backtrackCount;
    }
  }

  return hazards;
}

float calculateBeamDrainPerSecond(const BeamHazards &hazards,
                                  const BeamEnergyConfig &config) {
  const float rawDrain =
      static_cast<float>(hazards.selfCrossCount) *
          config.selfCrossDrainPerSecond +
      static_cast<float>(hazards.backtrackCount) *
          config.backtrackDrainPerSecond +
      static_cast<float>(hazards.directReturnCount) *
          config.directReturnDrainPerSecond;
  return std::clamp(rawDrain, 0.0f, config.maxDrainPerSecond);
}

float calculateLatchDrainPerSecond(int latchedCrawlerCount) {
  if (latchedCrawlerCount <= 0) {
    return 0.0f;
  }
  if (latchedCrawlerCount == 1) {
    return 3.0f;
  }
  if (latchedCrawlerCount == 2) {
    return 7.0f;
  }
  return 12.0f;
}

void updateBeamEnergy(BeamEnergyState &state, const BeamHazards &hazards,
                      const BeamEnergyConfig &config, float dt) {
  state.current = std::clamp(state.current, 0.0f, config.maxEnergy);
  const float targetCrossPressure =
      static_cast<float>(std::max(hazards.selfCrossCount, 0));
  const float crossPressureRate =
      targetCrossPressure > state.selfCrossPressure
          ? config.selfCrossPressureRisePerSecond
          : config.selfCrossPressureDecayPerSecond;
  state.selfCrossPressure =
      moveTowards(state.selfCrossPressure, targetCrossPressure,
                  std::max(crossPressureRate, 0.0f) * dt);

  state.drainPerSecond = calculateBeamDrainPerSecond(hazards, config);
  if (state.drainPerSecond > 0.0f) {
    state.current -= state.drainPerSecond * dt;
  } else {
    state.current += config.regenPerSecond * dt;
  }
  state.current = std::clamp(state.current, 0.0f, config.maxEnergy);
  state.danger = config.maxDrainPerSecond > 0.0f
                     ? std::clamp(state.drainPerSecond /
                                      config.maxDrainPerSecond,
                                  0.0f, 1.0f)
                     : 0.0f;
}

void addBeamEnergy(BeamEnergyState &state, const BeamEnergyConfig &config,
                   float amount) {
  state.current =
      std::clamp(state.current + std::max(amount, 0.0f), 0.0f,
                 std::max(config.maxEnergy, 0.0f));
}

} // namespace Deflektorish
