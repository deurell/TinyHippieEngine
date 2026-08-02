#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "meshnode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>

class KebnekaiseTerrainScene final : public DL::SceneNode {
public:
  KebnekaiseTerrainScene(
      DL::IRenderDevice *renderDevice,
      basist::etc1_global_selector_codebook *codeBook,
      DL::MeshAssetCache *meshAssetCache,
      DL::RenderResourceCache *renderResourceCache);

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "KebnekaiseTerrainScene";
  }

private:
  void updateCamera(float deltaTime, const DL::InputState &input);
  std::unique_ptr<MeshNode> createMeshNode(std::string path,
                                           std::string debugName,
                                           int renderLayer);

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  glm::vec3 cameraTarget_{0.0f, 4.0f, 0.0f};
  float cameraYaw_ = -0.48f;
  float cameraPitch_ = 0.38f;
  float cameraDistance_ = 43.0f;
  bool autoOrbit_ = true;
};
