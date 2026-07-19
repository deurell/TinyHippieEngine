#pragma once

#include <glm/glm.hpp>

namespace Deflektorish {

inline constexpr float kPixelToWorld = 0.01f;
inline constexpr glm::vec2 kScreenCenter{480.0f, 320.0f};
inline constexpr float kOrthographicHeight = 7.1f;

inline glm::vec2 gameToWorld(glm::vec2 pixels) {
  return (pixels - kScreenCenter) * kPixelToWorld;
}

} // namespace Deflektorish
