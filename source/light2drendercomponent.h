#pragma once

#include "renderdevice.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"
#include <string>

class Light2DNode;

namespace DL {

class Light2DRenderComponent : public RenderComponent {
public:
  explicit Light2DRenderComponent(
      Camera &camera, Light2DNode &node, IRenderDevice *renderDevice,
      RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/image.vert",
      std::string fragmentShaderPath = "Shaders/light2d.frag");
  ~Light2DRenderComponent() override;

  void render(const glm::mat4 &worldTransform, const FrameContext &ctx,
              RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "Light2DRenderComponent";
  }

private:
  Light2DNode &lightNode_;
  IRenderDevice *renderDevice_ = nullptr;
  RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
