#pragma once
#include <glm/glm.hpp>

namespace DL {
struct FrameContext {
  float delta_time = 0.0f;
  double total_time = 0.0;
};

class IScene {
public:
  virtual ~IScene() = default;

  virtual void init() = 0;
  virtual void update(const FrameContext &ctx) = 0;
  virtual void fixedUpdate(const FrameContext &ctx) {}
  virtual void render(const FrameContext &ctx) = 0;
  virtual void onClick(double x, double y) = 0;
  virtual void onKey(int key) = 0;
  virtual void onScreenSizeChanged(glm::vec2 size) = 0;
  virtual void onFramebufferSizeChanged(glm::vec2 size) {
    onScreenSizeChanged(size);
  }
};
} // namespace DL
