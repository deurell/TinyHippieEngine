#pragma once

#include "renderdevice.h"
#include "renderresourcecache.h"
#include "visualizerbase.h"

class ParticleSystemNode;

namespace DL {

class ParticleVisualizer : public VisualizerBase {
public:
  ParticleVisualizer(std::string name, DL::Camera &camera,
                     ParticleSystemNode &node, DL::IRenderDevice *renderDevice,
                     DL::RenderResourceCache *resourceCache = nullptr,
                     std::string vertexShaderPath = "Shaders/particlefx.vert",
                     std::string fragmentShaderPath = "Shaders/particlefx.frag");

  ~ParticleVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;

private:
  ParticleSystemNode &particleNode_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
