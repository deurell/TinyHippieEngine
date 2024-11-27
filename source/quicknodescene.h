#pragma once
#include "camera.h"
#include "scenenode.h"
#include <memory>

class PlaneNode;

class QuickNodeScene : public DL::SceneNode {
public:
  explicit QuickNodeScene(std::string_view glslVersionString);
  ~QuickNodeScene() override = default;
  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void bounce(float delta);

  std::string glslVersionString_;
  std::unique_ptr<DL::Camera> camera_ = nullptr;
  PlaneNode *plane_ = nullptr;
  glm::vec3 spritePosition{0, 0, 0};
};
