#pragma once

#include "basisu_global_selector_palette.h"
#include "meshassetcache.h"
#include "meshnode.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"

class MeshNodeScene : public DL::SceneNode {
public:
  explicit MeshNodeScene(DL::IRenderDevice *renderDevice = nullptr,
                         basist::etc1_global_selector_codebook *codeBook = nullptr,
                         DL::MeshAssetCache *meshAssetCache = nullptr,
                         DL::RenderResourceCache *renderResourceCache = nullptr);
  ~MeshNodeScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  MeshNode *meshNode_ = nullptr;
  bool debugNormals_ = false;
  DL::MeshVisualizerSettings visualizerSettings_;
};
