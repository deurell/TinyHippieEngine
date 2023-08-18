#pragma once
#include "icomponent.h"
#include "iscene.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <utility>
#include <vector>

namespace DL {

class SceneNode : public IScene {
public:
  SceneNode();
  glm::mat4 localTransform{1.0f};
  bool dirty{true};
  std::vector<std::unique_ptr<SceneNode>> children;
  std::vector<std::unique_ptr<IComponent>> components;

  // IScene methods
  void init() override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

  void updateTransforms(const glm::mat4 &parentWorldTransform);

  void setLocalPosition(const glm::vec3 &position);
  glm::vec3 getLocalPosition() const;

  void setLocalRotation(const glm::quat &rotation); // Changed to accept a quaternion
  glm::quat getLocalRotation() const;               // Changed to return a quaternion

  void setLocalScale(const glm::vec3 &scale);
  glm::vec3 getLocalScale() const;

private:
  glm::mat4 worldTransform{1.0f};

  glm::mat4 extractPositionMatrix() const; // Helper function
  glm::mat4 extractScaleMatrix() const;    // Helper function
};

} // namespace DL
