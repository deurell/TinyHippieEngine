#pragma once
#include "renderdevice.h"
#include "visualizerbase.h"
#include <glm/glm.hpp>
#include <string>

namespace DL {

class PlaneVisualizer : public VisualizerBase {
public:
  explicit PlaneVisualizer(
      std::string name, DL::Camera &camera, std::string_view glslVersionString,
      SceneNode &node, DL::IRenderDevice *renderDevice,
      std::string vertexShaderPath = "Shaders/simple.vert",
      std::string fragmentShaderPath = "Shaders/simple.frag");

  ~PlaneVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;
  glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  bool spinnerEnabled = false;
  float spinnerSpeed = 0.25f;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
