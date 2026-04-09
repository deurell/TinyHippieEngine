#pragma once
#include "renderdevice.h"
#include "visualizerbase.h"
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace DL {

class PlaneVisualizer : public VisualizerBase {
public:
  explicit PlaneVisualizer(
      std::string name, DL::Camera &camera, std::string_view glslVersionString,
      SceneNode &node, DL::IRenderDevice *renderDevice,
      const std::function<void(std::vector<DL::UniformValue> &)> &uniformModifier =
          nullptr,
      std::string vertexShaderPath = "Shaders/simple.vert",
      std::string fragmentShaderPath = "Shaders/simple.frag");

  ~PlaneVisualizer() override;

  void render(const glm::mat4 &worldTransform, float delta) override;
  glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
  std::function<void(std::vector<DL::UniformValue> &)> uniformModifier_;
};

} // namespace DL
