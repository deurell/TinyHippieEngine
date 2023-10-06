//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "plane.h"
#include "planenode.h"
#include "scenenode.h"
#include <memory>
#include <string_view>

class NodeExampleScene : public DL::SceneNode {
public:
  explicit NodeExampleScene(std::string_view glslVersionString);

  ~NodeExampleScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::string glslVersionString_;
  PlaneNode* plane2_;
  float scale_ = 1.0f;
  float rotation_ = 0.0f;
};
