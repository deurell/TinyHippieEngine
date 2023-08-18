//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "scenenode.h"
#include <string_view>

class LabNodeScene : public DL::SceneNode {
public:
  explicit LabNodeScene(std::string_view glslVersionString);

  ~LabNodeScene() override = default;

  void init() override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::string glslVersionString_;
};
