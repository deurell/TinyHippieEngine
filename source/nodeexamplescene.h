//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "planenode.h"
#include "scenenode.h"
#include <string_view>

class NodeExampleScene : public DL::SceneNode {
public:
  explicit NodeExampleScene(std::string_view glslVersionString);

  ~NodeExampleScene() override = default;

  void init() override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::string glslVersionString_;
};
