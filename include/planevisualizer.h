#pragma once
#include "visualizerbase.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace DL {

class PlaneVisualizer : public VisualizerBase {
public:
  explicit PlaneVisualizer(std::string name, DL::Camera &camera, std::string_view glslVersionString)
  : VisualizerBase(camera, std::move(name), std::string(glslVersionString), "Shaders/simple.vert", "Shaders/simple.frag") {
    camera.lookAt({0, 0, 0});

    auto shader = std::make_unique<DL::Shader>(vertexShaderPath_, fragmentShaderPath_, glslVersionString_);

    plane_ = std::make_unique<DL::Plane>(std::move(shader), camera_);
    plane_->position = {0, 0, 0};
    plane_->scale = {1.0, 1.0, 1.0};
    plane_->setRotation({0, 0, 0});
  }

  void init() override {}

  void render(const glm::mat4 &worldTransform, float delta) override {
    plane_->position = extractPosition(worldTransform);
    plane_->rotation = extractRotation(worldTransform);
    plane_->scale = extractScale(worldTransform);
    plane_->render(delta);
  }

private:
  std::unique_ptr<DL::Plane> plane_ = nullptr;
};

} // namespace DL
