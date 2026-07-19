#pragma once

#include <algorithm>

namespace Deflektorish {

struct FadeTransition {
  float duration = 0.45f;
  float time = 0.0f;
  bool active = false;

  void start(float newDuration) {
    duration = std::max(newDuration, 0.001f);
    time = 0.0f;
    active = true;
  }

  void update(float dt) {
    if (!active) {
      return;
    }
    time = std::min(time + std::max(dt, 0.0f), duration);
  }

  float progress() const {
    return std::clamp(time / std::max(duration, 0.001f), 0.0f, 1.0f);
  }

  float easedProgress() const {
    const float t = progress();
    return t * t * (3.0f - 2.0f * t);
  }

  float fadeInAlpha() const { return active ? easedProgress() : 0.0f; }
  float fadeOutAlpha() const { return active ? 1.0f - easedProgress() : 0.0f; }
  bool complete() const { return active && time >= duration; }
};

} // namespace Deflektorish
