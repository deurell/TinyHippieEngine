#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "tilemapvisualizer.h"
#include <string>

class TileMapNode : public DL::SceneNode {
public:
  TileMapNode(DL::TileMapConfig config, DL::IRenderDevice *renderDevice,
              DL::RenderResourceCache *renderResourceCache = nullptr,
              DL::SceneNode *parentNode = nullptr, DL::Camera *camera = nullptr);
  ~TileMapNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TileMapNode";
  }

private:
  void initCamera();
  void initComponents();

  DL::TileMapConfig config_;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
};
