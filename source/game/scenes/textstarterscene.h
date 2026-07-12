#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "cameranode.h"
#include "lightnode.h"
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
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TextStarterScene";
  }

private:
  void createFallbackCameraNode();
  void loadTextScene();
  void bindRuntimeNodes();
  void updateCameraController(const DL::FrameContext &ctx);
  void syncCameraControllerAngles();
  [[nodiscard]] DL::Camera *activeCamera();
  [[nodiscard]] const DL::Camera *activeCamera() const;

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::filesystem::path scenePath_;
  DL::LightingState lightingState_;
  DL::CameraNode *activeCameraNode_ = nullptr;
  DL::SceneNode *hero_ = nullptr;
  DL::SceneNode *hierarchyPlanetOrbit_ = nullptr;
  DL::SceneNode *hierarchyMoonOrbit_ = nullptr;
  DL::SceneNode *hierarchyMoon_ = nullptr;
  LightNode *sunLight_ = nullptr;
  float heroYawRadians_ = 0.0f;
  float planetOrbitRadians_ = 0.0f;
  float moonOrbitRadians_ = 0.0f;
  float moonSpinRadians_ = 0.0f;
  float cameraYaw_ = 0.0f;
  float cameraPitch_ = 0.0f;
};
