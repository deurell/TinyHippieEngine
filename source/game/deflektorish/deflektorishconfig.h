#pragma once

#include <glm/glm.hpp>

namespace Deflektorish {

inline constexpr float kPixelToWorld = 0.01f;
inline constexpr glm::vec2 kScreenCenter{480.0f, 320.0f};
inline constexpr float kOrthographicHeight = 7.1f;

inline glm::vec2 gameToWorld(glm::vec2 pixels) {
  return (pixels - kScreenCenter) * kPixelToWorld;
}

inline glm::vec2 gameToPostUv(glm::vec2 pixels, float viewportAspect) {
  const float safeAspect = viewportAspect > 0.0f ? viewportAspect : 16.0f / 9.0f;
  const float halfHeight = kOrthographicHeight * 0.5f;
  const float halfWidth = halfHeight * safeAspect;
  const glm::vec2 world = gameToWorld(pixels);
  return {0.5f + world.x / (halfWidth * 2.0f),
          0.5f + world.y / (halfHeight * 2.0f)};
}

} // namespace Deflektorish
