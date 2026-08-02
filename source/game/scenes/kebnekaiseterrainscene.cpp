#include "kebnekaiseterrainscene.h"

#include "debugui.h"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <string>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace {

constexpr char kTerrainPath[] =
    "Resources/Terrain/Kebnekaise/kebnekaise_imported_terrain.gltf";
constexpr char kRoutePath[] =
    "Resources/Terrain/Kebnekaise/kebnekaise_imported_route.gltf";
constexpr float kMouseSensitivity = 0.004f;
constexpr float kOrbitSpeed = 0.035f;
constexpr float kKeyboardOrbitSpeed = 0.72f;
constexpr float kZoomSpeed = 18.0f;

} // namespace

KebnekaiseTerrainScene::KebnekaiseTerrainScene(
    DL::IRenderDevice *renderDevice,
    basist::etc1_global_selector_codebook *codeBook,
    DL::MeshAssetCache *meshAssetCache,
    DL::RenderResourceCache *renderResourceCache)
    : renderDevice_(renderDevice), codeBook_(codeBook),
      meshAssetCache_(meshAssetCache),
      renderResourceCache_(renderResourceCache) {}

void KebnekaiseTerrainScene::init() {
  setDebugName("kebnekaise_terrain_scene");
  camera_ = std::make_unique<DL::Camera>();
  camera_->mFov = 38.0f;
  updateCamera(0.0f, {});

  addChild(createMeshNode(kTerrainPath, "kebnekaise_imported_terrain", 0));
  addChild(createMeshNode(kRoutePath, "kebnekaise_imported_route", 1));
  SceneNode::init();
}

std::unique_ptr<MeshNode> KebnekaiseTerrainScene::createMeshNode(
    std::string path, std::string debugName, int renderLayer) {
  auto node = std::make_unique<MeshNode>(
      std::move(path), codeBook_, renderDevice_, meshAssetCache_,
      renderResourceCache_, this, camera_.get());
  node->setDebugName(std::move(debugName));
  node->setRenderLayer(renderLayer);
  DL::MeshRenderSettings settings;
  settings.lightDirection = glm::normalize(glm::vec3(-0.45f, 1.0f, 0.28f));
  settings.lightColor = {1.0f, 0.94f, 0.82f};
  settings.ambientStrength = 0.72f;
  settings.specularStrength = 0.04f;
  settings.shininess = 8.0f;
  node->setRenderSettings(settings);
  node->init();
  return node;
}

void KebnekaiseTerrainScene::update(const DL::FrameContext &ctx) {
  updateCamera(ctx.delta_time, ctx.input);
  SceneNode::update(ctx);
}

void KebnekaiseTerrainScene::updateCamera(float deltaTime,
                                          const DL::InputState &input) {
  if (camera_ == nullptr) {
    return;
  }
  const bool dragging = input.isMouseButtonDown(DL::MouseButton::Right);
  if (dragging) {
    cameraYaw_ -= input.mouseDelta.x * kMouseSensitivity;
    cameraPitch_ = std::clamp(
        cameraPitch_ - input.mouseDelta.y * kMouseSensitivity, 0.12f, 1.18f);
  }
  if (std::abs(input.moveAxis.x) > 0.001f) {
    cameraYaw_ -= input.moveAxis.x * kKeyboardOrbitSpeed * deltaTime;
  } else if (autoOrbit_ && !dragging) {
    cameraYaw_ += kOrbitSpeed * deltaTime;
  }
  cameraDistance_ = std::clamp(
      cameraDistance_ - input.moveAxis.y * kZoomSpeed * deltaTime, 17.0f,
      72.0f);

  const float horizontalDistance = cameraDistance_ * std::cos(cameraPitch_);
  const glm::vec3 offset{horizontalDistance * std::sin(cameraYaw_),
                         cameraDistance_ * std::sin(cameraPitch_),
                         horizontalDistance * std::cos(cameraYaw_)};
  camera_->setPosition(cameraTarget_ + offset);
  camera_->lookAt(cameraTarget_);
}

void KebnekaiseTerrainScene::render(const DL::FrameContext &ctx) {
#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Kebnekaise terrain import");
  ImGui::TextUnformatted("Gazebo terrain import with Västra leden route");
  ImGui::TextUnformatted("Terrain asset:");
  ImGui::TextWrapped("%s", kTerrainPath);
  ImGui::TextUnformatted("A/D or stick left/right: orbit");
  ImGui::TextUnformatted("W/S or stick up/down: zoom");
  ImGui::TextUnformatted("Right-drag: free orbit");
  ImGui::Checkbox("Auto orbit", &autoOrbit_);
  ImGui::SliderFloat("Distance", &cameraDistance_, 17.0f, 72.0f);
  ImGui::End();
#endif
  SceneNode::render(ctx);
}

void KebnekaiseTerrainScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}
