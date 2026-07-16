#pragma once

#include "renderdevice.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"

class ParticleSystemNode;

namespace DL {

class ParticleRenderComponent : public RenderComponent {
public:
  ParticleRenderComponent(DL::Camera &camera, ParticleSystemNode &node,
                     DL::IRenderDevice *renderDevice,
                     DL::RenderResourceCache *resourceCache = nullptr,
                     std::string vertexShaderPath = "Shaders/particle.vert",
                     std::string fragmentShaderPath = "Shaders/particlefx.frag");

  ~ParticleRenderComponent() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "ParticleRenderComponent";
  }

  static glm::mat4 buildBillboardModel(const glm::vec3 &worldPosition,
                                       const glm::vec3 &scale,
                                       const DL::Camera &camera);

private:
  ParticleSystemNode &particleNode_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  bool billboardEnabled_ = true;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
