//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "plane.h"
#include "planenode.h"
#include "scenenode.h"
#include "textnode.h"

class NodeExampleScene : public DL::SceneNode {
public:
  explicit NodeExampleScene(std::string glslVersionString);
  ~NodeExampleScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void wrapScrollText();

  static constexpr glm::vec3 INITIAL_TEXT_POSITION = {-53, -24, 0};
  static constexpr float TEXT_RESET_POSITION = 100;

  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);

  std::string glslVersionString_;
  PlaneNode *plane1_ = nullptr;
  PlaneNode *plane2_ = nullptr;
  
  TextNode *textNode_ = nullptr;
  float scale_ = 1.0f;
  float rotation_ = 0.0f;
  float scrollAngle = 0;
  float scrollSpeed = 0.04f;
};
