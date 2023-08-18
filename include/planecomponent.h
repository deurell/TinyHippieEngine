#pragma once
#include "abstractcomponent.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace DL {

class PlaneComponent : public AbstractComponent {
public:
  explicit PlaneComponent(std::string name, DL::Camera &camera, std::string_view glslVersionString)
      : name_(std::move(name)), camera_(camera), glslVersionString_(glslVersionString) {
    camera.lookAt({0, 0, 0});
    auto shader = std::make_unique<DL::Shader>(
        "Shaders/simple.vert", "Shaders/simple.frag", glslVersionString_);

    plane_ = std::make_unique<DL::Plane>(std::move(shader), camera_);
    plane_->position = {0, 0, 0};
    plane_->scale = {1.0, 1.0, 1.0};
    plane_->setRotation({0, 0, 0});
  }
  void init() override {}

  void render(const glm::mat4 &worldTransform, float delta) override {
    plane_->position = glm::vec3(worldTransform[3]);
    glm::mat4 normalizedRotationMatrix = normalizeRotation(worldTransform);
    glm::quat rotationQuaternion = glm::quat_cast(normalizedRotationMatrix);
    plane_->rotation = rotationQuaternion;
    plane_->scale.x = glm::length(worldTransform[0]);
    plane_->scale.y = glm::length(worldTransform[1]);
    plane_->scale.z = glm::length(worldTransform[2]);
    plane_->render(delta);
  }

private:
  DL::Camera &camera_;
  std::string name_;
  std::unique_ptr<DL::Plane> plane_ = nullptr;
  std::string glslVersionString_;
};
}; // namespace DL
