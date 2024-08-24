#pragma once
#include "shader.h"
#include "texture.h"
#include "visualizerbase.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
namespace DL {

class ImageVisualizer : public VisualizerBase {
public:
  explicit ImageVisualizer(
      std::string name, DL::Camera &camera, std::string_view glslVersionString,
      SceneNode &node, std::unique_ptr<DL::Texture> texture, basist::etc1_global_selector_codebook *codeBook,
      const std::function<void(DL::Shader &)> &shaderModifier = nullptr,
      std::string vertexShaderPath = "Shaders/image.vert",
      std::string fragmentShaderPath = "Shaders/image.frag");

  void render(const glm::mat4 &worldTransform, float delta) override;

private:
  GLuint VAO_ = 0;
  GLuint VBO_ = 0;
  GLuint EBO_ = 0;

  const std::function<void(DL::Shader &)> shaderModifier_ = nullptr;
  std::unique_ptr<DL::Texture> texture_;
  basist::etc1_global_selector_codebook *codeBook_;
};

} // namespace DL
