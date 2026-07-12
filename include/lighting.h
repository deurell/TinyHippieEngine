#pragma once

#include <glm/glm.hpp>

namespace DL {

struct LightingState {
  bool directionalEnabled = false;
  glm::vec3 direction{0.35f, 1.0f, 0.25f};
  glm::vec3 color{1.0f, 0.96f, 0.9f};
  float intensity = 1.0f;
  float ambientStrength = 0.42f;
};

} // namespace DL
