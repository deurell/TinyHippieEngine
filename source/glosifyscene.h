//
// Created by Mikael Deurell on 2024-08-22.
//
#pragma once
#include "scenenode.h"

class PlaneNode;

namespace DL {
class GlosifyScene : public DL::SceneNode {
public:
  explicit GlosifyScene(std::string glslVersionString);
  ~GlosifyScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);

  std::string glslVersionString_;
  PlaneNode *plane_node_ = nullptr;
};

} // namespace DL