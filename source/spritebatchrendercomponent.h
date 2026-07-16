#pragma once

#include "renderdevice.h"
#include "renderqueue.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace DL {

struct SpriteBatchItem {
  glm::vec3 position{0.0f};
  glm::vec2 size{1.0f, 1.0f};
  float rotationDegrees = 0.0f;
  glm::vec4 sourceRectPixels{0.0f, 0.0f, -1.0f, -1.0f};
  bool flipX = false;
  bool flipY = false;
  bool flipDiagonal = false;
};

struct SpriteBatchConfig {
  std::string imagePath;
  std::vector<SpriteBatchItem> sprites;
};

class SpriteBatchRenderComponent : public RenderComponent {
public:
  SpriteBatchRenderComponent(
      DL::Camera &camera, SceneNode &node, SpriteBatchConfig config,
      DL::IRenderDevice *renderDevice,
      DL::RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/tilemap.vert",
      std::string fragmentShaderPath = "Shaders/tilemap.frag");

  ~SpriteBatchRenderComponent() override;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SpriteBatchRenderComponent";
  }

private:
  void buildMesh();

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  SpriteBatchConfig config_;
  MeshHandle mesh_;
  TextureHandle texture_;
  PipelineHandle pipeline_;
  bool sharedTexture_ = false;
  glm::vec2 atlasSize_{1.0f, 1.0f};
  Bounds localBounds_;
};

} // namespace DL
