#pragma once

#include "renderdevice.h"
#include "renderqueue.h"
#include "renderresourcecache.h"
#include "rendercomponent.h"
#include <cstdint>
#include <string>
#include <vector>

namespace DL {

struct TileMapTile {
  std::uint32_t tileIndex = 0;
  std::uint32_t x = 0;
  std::uint32_t y = 0;
  bool flipX = false;
  bool flipY = false;
  bool flipDiagonal = false;
};

struct TileMapLayer {
  std::string name;
  float z = 0.0f;
  std::vector<TileMapTile> tiles;
};

struct TileMapConfig {
  std::string imagePath;
  std::uint32_t firstGid = 1;
  std::uint32_t mapWidth = 0;
  std::uint32_t mapHeight = 0;
  std::uint32_t tileWidth = 16;
  std::uint32_t tileHeight = 16;
  std::uint32_t columns = 1;
  float tileWorldSize = 1.0f;
  std::vector<TileMapLayer> layers;
};

class TileMapRenderComponent : public RenderComponent {
public:
  TileMapRenderComponent(
      DL::Camera &camera, SceneNode &node, TileMapConfig config,
      DL::IRenderDevice *renderDevice,
      DL::RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/tilemap.vert",
      std::string fragmentShaderPath = "Shaders/tilemap.frag");

  ~TileMapRenderComponent() override;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TileMapRenderComponent";
  }

private:
  void buildMesh();

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  TileMapConfig config_;
  MeshHandle mesh_;
  TextureHandle texture_;
  PipelineHandle pipeline_;
  bool sharedTexture_ = false;
  glm::vec2 atlasSize_{1.0f, 1.0f};
  Bounds localBounds_;
};

} // namespace DL
