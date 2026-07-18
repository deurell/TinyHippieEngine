#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Deflektorish {

struct Reflektor {
  glm::vec2 position{0.0f};
  float angle = 0.0f;
};

struct Target {
  glm::vec2 position{0.0f};
  bool alive = true;
};

struct Blocker {
  glm::vec2 position{0.0f};
  bool reflective = false;
};

struct Portal {
  glm::vec2 entryPosition{0.0f};
  glm::vec2 exitPosition{0.0f};
};

struct Filter {
  glm::vec2 position{0.0f};
  float angle = 0.0f;
};

struct Splitter {
  glm::vec2 position{0.0f};
  float angle = 0.0f;
};

enum class BeamSegmentEnd {
  None,
  Reflektor,
  Target,
  Blocker,
  Portal,
  Filter,
  Splitter,
};

struct BeamSegment {
  glm::vec2 start{0.0f};
  glm::vec2 end{0.0f};
  float energy = 0.0f;
  BeamSegmentEnd endType = BeamSegmentEnd::None;
};

struct BeamSolveResult {
  std::vector<BeamSegment> segments;
  std::vector<bool> activeReflektors;
  std::vector<float> reflektorEnergy;
  std::vector<bool> activeBlockers;
  std::vector<float> blockerEnergy;
  std::vector<glm::vec2> blockerHit;
  std::vector<bool> blockerHasHit;
  std::vector<bool> hitTargets;
  std::vector<float> targetEnergy;
  std::vector<bool> activePortals;
  std::vector<glm::vec2> portalEntryHit;
  std::vector<glm::vec2> portalExitHit;
  std::vector<bool> portalHasHit;
  std::vector<bool> passingFilters;
  std::vector<bool> blockedFilters;
  std::vector<glm::vec2> filterHit;
  std::vector<bool> filterHasHit;
  std::vector<bool> activeSplitters;
  std::vector<glm::vec2> splitterHit;
  std::vector<bool> splitterHasHit;
};

struct BeamWorld {
  glm::vec2 sourcePosition{0.0f};
  float sourceAngle = 0.0f;
  std::vector<Reflektor> reflektors;
  std::vector<Target> targets;
  std::vector<Blocker> blockers;
  std::vector<Portal> portals;
  std::vector<Filter> filters;
  std::vector<Splitter> splitters;
};

BeamSolveResult solveBeamWorld(const BeamWorld &world,
                               std::size_t maxSegments);

} // namespace Deflektorish
