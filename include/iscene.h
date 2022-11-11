#pragma once
#include <glm/glm.hpp>

namespace DL {
class IScene {
public:
  virtual ~IScene() = default;

  virtual void init() = 0;
  virtual void render(float delta) = 0;
  virtual void onClick() = 0;
  virtual void onKey(int key) = 0;
  virtual void onScreenSizeChanged(glm::vec2 size) = 0;
};
} // namespace DL
