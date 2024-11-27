#pragma once
#include "scenenode.h"

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
  std::string glslVersionString_;
  PlaneNode* plane_ = nullptr;
};
