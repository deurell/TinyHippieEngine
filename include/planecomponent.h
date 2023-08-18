#pragma once
#include "icomponent.h"
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>

namespace DL {

class PlaneComponent : public IComponent {
public:
  explicit PlaneComponent(std::string name, DL::Camera &camera, std::string_view glslVersionString)
      : name_(std::move(name)), camera_(camera), glslVersionString_(glslVersionString) {
    camera.lookAt({0, 0, 0});
    auto shader = std::make_unique<DL::Shader>(
        "Shaders/simple.vert", "Shaders/simple.frag", glslVersionString_);

    plane_ = std::make_unique<DL::Plane>(std::move(shader), camera_);
    plane_->position = {0, 0, 0};
    plane_->scale = {1, 6, 1};
    plane_->setRotation({0,0,glm::pi<float>()/2});
  }
  void init() override {}

  void render(const glm::mat4 &worldTransform, float delta) override {
    plane_->position = worldTransform[3];
    plane_->setRotation({0,0,glm::pi<float>()/4});
    plane_->render(delta);
  }

private:
  DL::Camera &camera_;
  std::string name_;
  std::unique_ptr<DL::Plane> plane_ = nullptr;
  std::string glslVersionString_;
};
}; // namespace DL
