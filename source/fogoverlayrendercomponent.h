#pragma once

#include "renderdevice.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"
#include <string>

class FogOverlayNode;

namespace DL {

class FogOverlayRenderComponent : public RenderComponent {
public:
  FogOverlayRenderComponent(
      Camera &camera, FogOverlayNode &node, IRenderDevice *renderDevice,
      RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/fogoverlay.vert",
      std::string fragmentShaderPath = "Shaders/fogoverlay.frag");
  ~FogOverlayRenderComponent() override;

  void render(const glm::mat4 &worldTransform, const FrameContext &ctx,
              RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "FogOverlayRenderComponent";
  }

private:
  FogOverlayNode &fogNode_;
  IRenderDevice *renderDevice_ = nullptr;
  RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  TextureHandle texture_;
  PipelineHandle pipeline_;
  bool sharedTexture_ = false;
};

} // namespace DL
