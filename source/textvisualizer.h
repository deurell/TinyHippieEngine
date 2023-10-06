#pragma once
#include "camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "shader.h"
#include "textsprite.h"
#include "visualizerbase.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace DL {

class TextVisualizer : public VisualizerBase {

public:
  explicit TextVisualizer(std::string name, DL::Camera &camera,
                          std::string_view glslVersionString, SceneNode &node,
                          std::string_view text)
      : VisualizerBase(camera, std::move(name), std::string(glslVersionString),
                       "Shaders/status.vert", "Shaders/status.frag", node),
        text_(text) {

    camera.lookAt({0, 0, 0});
    textSprite_ =
        std::make_unique<DL::TextSprite>("Resources/C64_Pro-STYLE.ttf", text_);
    shader_ = std::make_unique<DL::Shader>(
        vertexShaderPath_, fragmentShaderPath_, glslVersionString_);
  }

  void init() override {}

  void render(const glm::mat4 &worldTransform, float delta) override {
    shader_->use();
    glm::mat4 model = glm::mat4(1.0f);
    auto position = extractPosition(worldTransform);
    auto rotation = extractRotation(worldTransform);
    auto scale = extractScale(worldTransform);

    model = glm::translate(model, position);
    model = model * glm::mat4_cast(rotation);
    model = glm::scale(model, glm::vec3(0.03, 0.03, 1.0) * scale);

    shader_->setMat4f("model", model);
    glm::mat4 view = camera_.getViewMatrix();
    shader_->setMat4f("view", view);
    glm::mat4 projectionMatrix = camera_.getPerspectiveTransform();
    shader_->setMat4f("projection", projectionMatrix);

    shader_->setFloat("iTime", static_cast<float>(glfwGetTime()));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textSprite_->mFontTexture);
    shader_->setInt("texture1", 0);
    textSprite_->render(delta);
  }

private:
  std::unique_ptr<DL::TextSprite> textSprite_ = nullptr;
  std::string_view text_;
  std::unique_ptr<DL::Shader> shader_ = nullptr;
};

} // namespace DL