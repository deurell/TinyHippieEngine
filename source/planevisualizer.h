#pragma once
#include "plane.h"
#include "shader.h"
#include "visualizerbase.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace DL {

class PlaneVisualizer : public VisualizerBase {
public:
  explicit PlaneVisualizer(
      std::string name, DL::Camera &camera, std::string_view glslVersionString,
      SceneNode &node,
      const std::function<void(DL::Shader &)> &shaderModifier = nullptr,
      std::string vertexShaderPath = "Shaders/simple.vert",
      std::string fragmentShaderPath = "Shaders/simple.frag");

  void render(const glm::mat4 &worldTransform, float delta) override;

private:
  GLuint VAO_ = 0;
  GLuint VBO_ = 0;
  GLuint EBO_ = 0;

  const std::function<void(DL::Shader &)> shaderModifier_ = nullptr;
};

} // namespace DL
