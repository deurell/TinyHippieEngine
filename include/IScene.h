#pragma once
#include <glm/glm.hpp>

class IScene {
public:
  virtual ~IScene(){};

  virtual void init() = 0;
  virtual void render(float delta) = 0;
  virtual void onKey(int key, float delta) = 0;
  virtual void onScreenSizeChanged(glm::vec2 size) = 0;
};