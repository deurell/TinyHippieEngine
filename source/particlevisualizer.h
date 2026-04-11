#pragma once

#include "renderdevice.h"
#include "visualizerbase.h"

class ParticleSystemNode;

namespace DL {

class ParticleVisualizer : public VisualizerBase {
public:
  ParticleVisualizer(std::string name, DL::Camera &camera,
                     ParticleSystemNode &node, DL::IRenderDevice *renderDevice,
                     std::string vertexShaderPath = "Shaders/particlefx.vert",
                     std::string fragmentShaderPath = "Shaders/particlefx.frag");

  ~ParticleVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;

private:
  ParticleSystemNode &particleNode_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
