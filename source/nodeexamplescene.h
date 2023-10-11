//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "plane.h"
#include "planenode.h"
#include "scenenode.h"
#include "textnode.h"
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
  static constexpr glm::vec3 INITIAL_TEXT_POSITION = {-53, -24, 0};
  static constexpr float TEXT_RESET_POSITION = 100;

  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);

  std::string glslVersionString_;
  PlaneNode *plane1_;
  PlaneNode *plane2_;
  TextNode *textNode_;
  float scale_ = 1.0f;
  float rotation_ = 0.0f;
  float scrollAngle = 0;
};
