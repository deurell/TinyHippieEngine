#pragma once

#include "renderdevice.h"
#include <glm/glm.hpp>
#include <vector>

namespace DL {

struct FrameContext;

enum class RenderTag {
  Opaque,
  Sprite,
  SpriteBatch,
  TileMap,
  Text,
  Particle,
  Overlay,
  PostProcess,
};

struct Bounds {
  glm::vec3 center{0.0f};
  glm::vec3 halfExtents{0.0f};
};

struct RenderItem {
  RenderTag tag = RenderTag::Opaque;
  Bounds localBounds;
  bool cullable = true;
  MeshHandle mesh;
  PipelineHandle pipeline;
  TextureHandle texture;
  RenderPassId pass = RenderPassId::Opaque;
  BlendMode blendMode = BlendMode::Opaque;
  bool depthTest = true;
  int renderLayer = 0;
  DrawSortMode sortMode = DrawSortMode::None;
  float sortDepth = 0.0f;
  float lineWidth = 1.0f;
  std::vector<UniformValue> uniforms;
};

struct RenderQueueStats {
  std::uint32_t submittedItems = 0;
  std::uint32_t drawnItems = 0;
  std::uint32_t culledItems = 0;
};

struct RenderQueueOptions {
  bool cullingEnabled = true;
};

[[nodiscard]] DrawCommand toDrawCommand(const RenderItem &item);
void submitRenderItem(const FrameContext &ctx, IRenderDevice &renderDevice,
                      RenderItem item);

class RenderQueue {
public:
  void submit(RenderItem item);
  RenderQueueStats flush(IRenderDevice &renderDevice,
                         RenderQueueOptions options = {});
  void clear();
  [[nodiscard]] bool empty() const { return items_.empty(); }
  [[nodiscard]] std::size_t size() const { return items_.size(); }

private:
  std::vector<RenderItem> items_;
};

} // namespace DL
