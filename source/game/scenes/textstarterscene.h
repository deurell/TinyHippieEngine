#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenedescription.h"
#include "scenenode.h"
#include <filesystem>
#include <memory>
#include <string_view>

class TextStarterScene : public DL::SceneNode {
public:
  explicit TextStarterScene(
      DL::IRenderDevice *renderDevice = nullptr,
      basist::etc1_global_selector_codebook *codeBook = nullptr,
      DL::MeshAssetCache *meshAssetCache = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr,
      std::filesystem::path scenePath =
          "Resources/Scenes/simple_starter.scene.json");
  ~TextStarterScene() override = default;

  void init() override;
  void fixedUpdate(const DL::FrameContext &ctx) override;
  void update(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TextStarterScene";
  }

private:
  void initCamera();
  void loadTextScene();
  void bindRuntimeNodes();

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::filesystem::path scenePath_;
  std::unique_ptr<DL::Camera> camera_;
  DL::SceneNode *hero_ = nullptr;
  float heroYawRadians_ = 0.0f;
};
