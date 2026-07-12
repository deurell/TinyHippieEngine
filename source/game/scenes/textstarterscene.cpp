#include "textstarterscene.h"

#include "logger.h"
#include "meshnode.h"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>

namespace {

constexpr float kCameraMoveSpeed = 4.2f;
constexpr float kCameraMouseSensitivity = 0.0035f;
constexpr float kCameraMaxPitch = 1.2f;

glm::vec3 cameraForward(float yaw, float pitch) {
  const float cosPitch = std::cos(pitch);
  return glm::normalize(
      glm::vec3(std::sin(yaw) * cosPitch, std::sin(pitch),
                std::cos(yaw) * cosPitch));
}

const DL::SceneNodeDescription *
findActiveRootCameraDescription(const DL::SceneDescription &description) {
  for (const auto &node : description.nodes) {
    if (node.type == "CameraNode" && node.active) {
      return &node;
    }
  }
  return nullptr;
}

} // namespace

TextStarterScene::TextStarterScene(
    DL::IRenderDevice *renderDevice,
    basist::etc1_global_selector_codebook *codeBook,
    DL::MeshAssetCache *meshAssetCache,
    DL::RenderResourceCache *renderResourceCache,
    std::filesystem::path scenePath)
    : SceneNode(nullptr), renderDevice_(renderDevice), codeBook_(codeBook),
      meshAssetCache_(meshAssetCache),
      renderResourceCache_(renderResourceCache),
      scenePath_(std::move(scenePath)) {
  setDebugName("text starter scene");
}

void TextStarterScene::init() {
  SceneNode::init();
  loadTextScene();
  bindRuntimeNodes();
}

void TextStarterScene::fixedUpdate(const DL::FrameContext &ctx) {
  if (dynamic_cast<MeshNode *>(hero_) != nullptr) {
    heroYawRadians_ += ctx.delta_time * 0.45f;
    hero_->setLocalRotation(glm::quat(glm::vec3(0.0f, heroYawRadians_, 0.0f)));
  }
  if (hierarchyPlanetOrbit_ != nullptr) {
    planetOrbitRadians_ += ctx.delta_time * 0.7f;
    hierarchyPlanetOrbit_->setLocalRotation(
        glm::quat(glm::vec3(0.0f, planetOrbitRadians_, 0.0f)));
  }
  if (hierarchyMoonOrbit_ != nullptr) {
    moonOrbitRadians_ += ctx.delta_time * 2.2f;
    hierarchyMoonOrbit_->setLocalRotation(
        glm::quat(glm::vec3(0.0f, moonOrbitRadians_, 0.0f)));
  }
  if (hierarchyMoon_ != nullptr) {
    moonSpinRadians_ += ctx.delta_time * 3.1f;
    hierarchyMoon_->setLocalRotation(
        glm::quat(glm::vec3(0.0f, moonSpinRadians_, 0.0f)));
  }
  SceneNode::fixedUpdate(ctx);
}

void TextStarterScene::update(const DL::FrameContext &ctx) {
  updateCameraController(ctx);
  lightingState_ = {};
  DL::FrameContext lightingCtx = ctx;
  lightingCtx.lighting = &lightingState_;
  SceneNode::update(lightingCtx);
}

void TextStarterScene::render(const DL::FrameContext &ctx) {
  DL::FrameContext lightingCtx = ctx;
  lightingCtx.lighting = &lightingState_;
  SceneNode::render(lightingCtx);
}

void TextStarterScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

void TextStarterScene::createFallbackCameraNode() {
  auto cameraNode = std::make_unique<DL::CameraNode>(this);
  cameraNode->setDebugName("fallback_camera");
  cameraNode->setActive(true);
  cameraNode->setFov(40.0f);
  cameraNode->setLocalPosition({4.7f, 3.0f, 5.7f});
  cameraNode->setLookAtTarget({0.0f, 0.75f, 0.15f});
  cameraNode->init();
  activeCameraNode_ = cameraNode.get();
  addChild(std::move(cameraNode));
  syncCameraControllerAngles();
}

void TextStarterScene::loadTextScene() {
  const DL::SceneDescription description = DL::loadSceneDescription(scenePath_);
  const DL::SceneNodeDescription *activeCameraDescription =
      findActiveRootCameraDescription(description);

  std::string activeCameraName;
  if (activeCameraDescription != nullptr) {
    activeCameraName = activeCameraDescription->name;
    addChild(DL::buildSceneNode(*activeCameraDescription,
                                {.renderDevice = renderDevice_,
                                 .codeBook = codeBook_,
                                 .meshAssetCache = meshAssetCache_,
                                 .renderResourceCache = renderResourceCache_,
                                 .camera = nullptr},
                                this));
    activeCameraNode_ = dynamic_cast<DL::CameraNode *>(children.back().get());
    if (activeCameraNode_ != nullptr) {
      syncCameraControllerAngles();
    }
  }

  if (activeCameraNode_ == nullptr) {
    createFallbackCameraNode();
  }

  DL::Camera *sceneCamera = activeCamera();
  for (const auto &nodeDescription : description.nodes) {
    if (!activeCameraName.empty() && nodeDescription.name == activeCameraName) {
      continue;
    }
    addChild(DL::buildSceneNode(
        nodeDescription,
        {.renderDevice = renderDevice_,
         .codeBook = codeBook_,
         .meshAssetCache = meshAssetCache_,
         .renderResourceCache = renderResourceCache_,
         .camera = sceneCamera},
        this));
  }
  DL::Logger::instance().logEvent(DL::LogLevel::Info, "scene_description",
                                  "loaded " + description.name);
}

void TextStarterScene::bindRuntimeNodes() {
  hero_ = DL::findSceneNodeByName(*this, "hero");
  if (hero_ == nullptr) {
    throw std::runtime_error(
        "TextStarterScene requires a node named 'hero' for C++ behavior");
  }
  hierarchyPlanetOrbit_ = DL::findSceneNodeByName(*this, "hierarchy_planet_orbit");
  hierarchyMoonOrbit_ = DL::findSceneNodeByName(*this, "hierarchy_moon_orbit");
  hierarchyMoon_ = DL::findSceneNodeByName(*this, "hierarchy_moon");
  sunLight_ =
      dynamic_cast<LightNode *>(DL::findSceneNodeByName(*this, "sun_light"));
}

void TextStarterScene::updateCameraController(const DL::FrameContext &ctx) {
  DL::Camera *camera = activeCamera();
  if (activeCameraNode_ == nullptr || camera == nullptr) {
    return;
  }

  if (ctx.input.isMouseButtonDown(DL::MouseButton::Right)) {
    cameraYaw_ -= ctx.input.mouseDelta.x * kCameraMouseSensitivity;
    cameraPitch_ =
        std::clamp(cameraPitch_ - ctx.input.mouseDelta.y * kCameraMouseSensitivity,
                   -kCameraMaxPitch, kCameraMaxPitch);
  }

  const glm::vec3 forward = cameraForward(cameraYaw_, cameraPitch_);
  activeCameraNode_->lookAtWorld(camera->getPosition() + forward);

  const float movementStep = kCameraMoveSpeed * ctx.delta_time;
  if (ctx.input.isActionDown(DL::Action::MoveForward)) {
    activeCameraNode_->translateLocal({0.0f, 0.0f, -movementStep});
  }
  if (ctx.input.isActionDown(DL::Action::MoveBackward)) {
    activeCameraNode_->translateLocal({0.0f, 0.0f, movementStep});
  }
  if (ctx.input.isActionDown(DL::Action::MoveRight)) {
    activeCameraNode_->translateLocal({movementStep, 0.0f, 0.0f});
  }
  if (ctx.input.isActionDown(DL::Action::MoveLeft)) {
    activeCameraNode_->translateLocal({-movementStep, 0.0f, 0.0f});
  }
}

void TextStarterScene::syncCameraControllerAngles() {
  const DL::Camera *camera = activeCamera();
  if (camera == nullptr) {
    return;
  }

  const glm::vec3 forward = glm::normalize(
      glm::vec3(glm::inverse(glm::mat4_cast(camera->mOrientation)) *
                glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
  cameraYaw_ = std::atan2(forward.x, forward.z);
  cameraPitch_ = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
}

DL::Camera *TextStarterScene::activeCamera() {
  return activeCameraNode_ != nullptr ? &activeCameraNode_->camera() : nullptr;
}

const DL::Camera *TextStarterScene::activeCamera() const {
  return activeCameraNode_ != nullptr ? &activeCameraNode_->camera() : nullptr;
}
