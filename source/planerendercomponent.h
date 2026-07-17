#pragma once
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"
#include <glm/glm.hpp>
#include <string>

namespace DL {

class PlaneRenderComponent : public RenderComponent {
public:
  explicit PlaneRenderComponent(
      DL::Camera &camera, SceneNode &node, DL::IRenderDevice *renderDevice,
      DL::RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/simple.vert",
      std::string fragmentShaderPath = "Shaders/simple.frag");

  ~PlaneRenderComponent() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PlaneRenderComponent";
  }
  glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  bool spinnerEnabled = false;
  float spinnerSpeed = 0.25f;
  int proceduralStyle = 0;
  glm::vec4 proceduralParams{0.0f, 0.0f, 0.0f, 0.0f};
  glm::vec4 proceduralParams2{0.0f, 0.0f, 0.0f, 0.0f};
  DL::BlendMode blendMode = DL::BlendMode::Opaque;
  bool depthTest = true;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
