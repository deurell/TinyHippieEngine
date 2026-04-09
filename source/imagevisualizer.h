#pragma once
#include "renderdevice.h"
#include "visualizerbase.h"
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
namespace DL {

class ImageVisualizer : public VisualizerBase {
public:
  explicit ImageVisualizer(
      std::string name, DL::Camera &camera, std::string_view glslVersionString,
      SceneNode &node, std::string texturePath,
      basist::etc1_global_selector_codebook *codeBook, DL::IRenderDevice *renderDevice,
      std::string vertexShaderPath = "Shaders/image.vert",
      std::string fragmentShaderPath = "Shaders/image.frag");

  ~ImageVisualizer() override;
  void render(const glm::mat4 &worldTransform, float delta) override;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  TextureHandle texture_;
  PipelineHandle pipeline_;
  std::string texturePath_;
  basist::etc1_global_selector_codebook *codeBook_;
};

} // namespace DL
