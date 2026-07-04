#include "physicstestscene.h"

#include "debugui.h"
#include "physicsdebugvisualizer.h"
#include "physicsmeshvisualizer.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace {

constexpr float kCameraMoveSpeed = 5.5f;
constexpr float kCameraMouseSensitivity = 0.0035f;
constexpr float kCameraMaxPitch = 1.22f;

glm::vec3 cameraForward(float yaw, float pitch) {
  const float cosPitch = std::cos(pitch);
  return glm::normalize(
      glm::vec3(std::sin(yaw) * cosPitch, std::sin(pitch),
                std::cos(yaw) * cosPitch));
}

} // namespace

PhysicsTestScene::PhysicsTestScene(DL::IRenderDevice *renderDevice)
    : SceneNode(nullptr), renderDevice_(renderDevice) {}

void PhysicsTestScene::init() {
  SceneNode::init();
  initCamera();
  initWorld();
  initHitMarker();

  if (renderDevice_ != nullptr) {
    addRenderComponent(std::make_unique<PhysicsDebugVisualizer>(
        camera_, *this, *renderDevice_, physicsContext_, markerLines_));
  }
}

void PhysicsTestScene::initCamera() {
  camera_.mFov = 40.0f;
  camera_.lookAt(cameraTarget_);
  const glm::vec3 toTarget =
      glm::normalize(cameraTarget_ - camera_.getPosition());
  cameraYaw_ = std::atan2(toTarget.x, toTarget.z);
  cameraPitch_ = std::asin(std::clamp(toTarget.y, -1.0f, 1.0f));
}

DL::PhysicsBodyHandle PhysicsTestScene::addBody(DL::PhysicsBodyDesc desc,
                                                const glm::vec4 &color) {
  if (desc.type == DL::PhysicsBodyType::Dynamic) {
    desc.enableSleep = false;
    desc.startAwake = true;
  }

  const auto handle = physicsContext_.world().createBody(desc);
  if (!handle.valid()) {
    return {};
  }
  DL::SceneNode *nodePtr = nullptr;
  if (renderDevice_ != nullptr) {
    auto node = std::make_unique<DL::SceneNode>(this);
    node->init();
    node->setDebugName("physics body");
    node->setLocalPosition(desc.position);
    node->setLocalRotation(desc.rotation);
    node->addRenderComponent(std::make_unique<PhysicsMeshVisualizer>(
        camera_, *node, *renderDevice_, desc.shape, color));
    nodePtr = node.get();
    addChild(std::move(node));
  }
  bodies_.push_back({.handle = handle,
                     .type = desc.type,
                     .node = nodePtr,
                     .spawnPosition = desc.position,
                     .spawnRotation = desc.rotation});
  return handle;
}

void PhysicsTestScene::initHitMarker() {
  if (renderDevice_ == nullptr) {
    return;
  }

  auto marker = std::make_unique<DL::SceneNode>(this);
  marker->init();
  marker->setDebugName("physics hit marker");
  marker->setLocalPosition({0.0f, -1000.0f, 0.0f});
  marker->addRenderComponent(std::make_unique<PhysicsMeshVisualizer>(
      camera_, *marker, *renderDevice_, DL::PhysicsShapeDesc::makeSphere(0.16f),
      glm::vec4{1.0f, 0.95f, 0.05f, 1.0f}));
  hitMarkerNode_ = marker.get();
  addChild(std::move(marker));
}

void PhysicsTestScene::initWorld() {
  bodies_.clear();
  physicsContext_.setGravity({0.0f, -9.81f, 0.0f});
  physicsContext_.setDebugRenderingEnabled(debugLinesEnabled_);
  physicsContext_.setDebugRenderSettings(
      {.collisionShapes = true,
       .velocityVectors = velocityDebugEnabled_,
       .velocityScale = 0.18f});

  const glm::vec4 floorColor{0.36f, 0.39f, 0.42f, 1.0f};
  const glm::vec4 railColor{0.22f, 0.25f, 0.28f, 1.0f};
  const glm::vec4 rampColor{0.15f, 0.42f, 0.74f, 1.0f};
  const glm::vec4 boxColor{0.96f, 0.48f, 0.18f, 1.0f};
  const glm::vec4 sphereColor{0.12f, 0.74f, 0.68f, 1.0f};
  const glm::vec4 capsuleColor{0.42f, 0.74f, 0.24f, 1.0f};
  const glm::vec4 pusherColor{0.82f, 0.22f, 0.72f, 1.0f};
  const glm::vec4 platformColor{0.72f, 0.34f, 0.92f, 1.0f};
  const glm::vec4 testCubeColor{1.0f, 0.18f, 0.14f, 1.0f};

  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({7.0f, 0.25f, 3.8f}),
           .position = {0.0f, -0.25f, 0.0f},
           .categoryBits = 0x0001,
           .maskBits = 0xFFFF},
          floorColor);

  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({0.18f, 0.25f, 3.6f}),
           .position = {-4.3f, 0.25f, 0.0f}},
          railColor);
  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({0.18f, 0.25f, 3.6f}),
           .position = {4.3f, 0.25f, 0.0f}},
          railColor);
  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({4.0f, 0.22f, 0.18f}),
           .position = {0.0f, 0.22f, -3.1f}},
          railColor);
  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({2.6f, 0.22f, 0.18f}),
           .position = {-1.2f, 0.22f, 3.1f}},
          railColor);

  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({1.45f, 0.18f, 0.75f}),
           .position = {2.65f, 0.62f, -1.05f},
           .rotation = glm::angleAxis(glm::radians(18.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f))},
          rampColor);
  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({1.35f, 0.16f, 0.55f}),
           .position = {-2.95f, 0.55f, 1.65f},
           .rotation = glm::angleAxis(glm::radians(-16.0f),
                                      glm::vec3(0.0f, 0.0f, 1.0f))},
          rampColor);

  pusher_ = addBody({.type = DL::PhysicsBodyType::Kinematic,
                     .shape =
                         DL::PhysicsShapeDesc::makeBox({0.22f, 0.45f, 1.05f}),
                     .position = {-3.0f, 0.45f, -0.55f},
                     .categoryBits = 0x0001,
                     .maskBits = 0xFFFF},
                    pusherColor);

  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < 5 - row; ++col) {
      addBody({.type = DL::PhysicsBodyType::Dynamic,
               .shape = DL::PhysicsShapeDesc::makeBox({0.26f, 0.26f, 0.26f}),
               .position = {-0.95f + col * 0.58f + row * 0.29f,
                            0.32f + row * 0.56f, -0.95f}},
              boxColor);
    }
  }

  for (int i = 0; i < 5; ++i) {
    addBody({.type = DL::PhysicsBodyType::Dynamic,
             .shape = DL::PhysicsShapeDesc::makeSphere(0.28f),
             .position = {1.15f + i * 0.34f, 2.65f + i * 0.18f, -2.1f},
             .linearVelocity = {-0.55f, 0.0f, 0.65f}},
            sphereColor);
  }

  for (int i = 0; i < 4; ++i) {
    addBody({.type = DL::PhysicsBodyType::Dynamic,
             .shape = DL::PhysicsShapeDesc::makeCapsule(0.18f, 0.75f),
             .position = {1.35f + i * 0.45f, 2.05f + i * 0.34f, 1.15f},
             .rotation = glm::angleAxis(glm::radians(90.0f),
                                        glm::vec3(1.0f, 0.0f, 0.0f))},
            capsuleColor);
  }

  addBody({.type = DL::PhysicsBodyType::Static,
           .shape = DL::PhysicsShapeDesc::makeBox({1.45f, 0.12f, 0.65f}),
           .position = {-0.2f, 1.05f, 2.15f},
           .categoryBits = 0x0001,
           .maskBits = 0xFFFF},
          platformColor);
  addBody({.type = DL::PhysicsBodyType::Dynamic,
           .shape = DL::PhysicsShapeDesc::makeBox({0.26f, 0.26f, 0.26f}),
           .position = {-0.2f, 2.75f, 2.15f},
           .categoryBits = 0x0001,
           .maskBits = 0xFFFF},
          testCubeColor);

  syncVisualBodies();
}

void PhysicsTestScene::update(const DL::FrameContext &ctx) {
  updateCameraController(ctx);
  const bool isLeftMouseDown =
      ctx.input.isMouseButtonDown(DL::MouseButton::Left);
  if (isLeftMouseDown && !wasLeftMouseDown_) {
    pickPhysicsBody(ctx.input.sceneMousePosition);
  }
  wasLeftMouseDown_ = isLeftMouseDown;
  SceneNode::update(ctx);
}

void PhysicsTestScene::fixedUpdate(const DL::FrameContext &ctx) {
  if (simulationEnabled_) {
    updateKinematicPusher(ctx.total_time);
    physicsContext_.step(static_cast<float>(ctx.delta_time));
    if (autoSpawnEnabled_) {
      spawnAccumulator_ += static_cast<float>(ctx.delta_time);
      if (spawnAccumulator_ >= 0.28f) {
        spawnAccumulator_ = 0.0f;
        spawnDynamicBody();
      }
    }
    syncVisualBodies();
  }
  SceneNode::fixedUpdate(ctx);
}

void PhysicsTestScene::updateKinematicPusher(double totalTime) {
  if (!pusher_.valid()) {
    return;
  }
  const float x = -3.0f + std::sin(static_cast<float>(totalTime) * 1.2f) * 1.55f;
  physicsContext_.world().setBodyTransform(
      pusher_, {x, 0.45f, -0.55f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
  physicsContext_.world().setLinearVelocity(
      pusher_, {std::cos(static_cast<float>(totalTime) * 1.2f) * 1.86f, 0.0f,
                0.0f});
}

void PhysicsTestScene::syncVisualBodies() {
  for (auto &body : bodies_) {
    if (body.node == nullptr || !body.handle.valid()) {
      continue;
    }
    const auto state = physicsContext_.world().getBodyState(body.handle);
    body.node->setLocalPosition(state.position);
    body.node->setLocalRotation(state.rotation);
  }
}

void PhysicsTestScene::spawnBurst(int count) {
  for (int i = 0; i < count; ++i) {
    spawnDynamicBody();
  }
}

void PhysicsTestScene::spawnDynamicBody() {
  if (bodies_.size() > 140) {
    return;
  }

  const int index = spawnCounter_++;
  const float x = -1.2f + static_cast<float>(index % 5) * 0.55f;
  const float z = -0.45f + static_cast<float>((index / 5) % 5) * 0.35f;
  const float y = 4.2f + static_cast<float>(index % 4) * 0.35f;
  const glm::vec3 velocity{0.45f * std::sin(static_cast<float>(index) * 1.7f),
                           0.0f,
                           0.35f * std::cos(static_cast<float>(index) * 1.3f)};

  const int shape = index % 3;
  if (shape == 0) {
    addBody({.type = DL::PhysicsBodyType::Dynamic,
             .shape = DL::PhysicsShapeDesc::makeBox({0.24f, 0.24f, 0.24f}),
             .position = {x, y, z},
             .linearVelocity = velocity},
            {0.98f, 0.58f, 0.2f, 1.0f});
  } else if (shape == 1) {
    addBody({.type = DL::PhysicsBodyType::Dynamic,
             .shape = DL::PhysicsShapeDesc::makeSphere(0.25f),
             .position = {x, y, z},
             .linearVelocity = velocity},
            {0.1f, 0.78f, 0.88f, 1.0f});
  } else {
    addBody({.type = DL::PhysicsBodyType::Dynamic,
             .shape = DL::PhysicsShapeDesc::makeCapsule(0.16f, 0.65f),
             .position = {x, y, z},
             .rotation = glm::angleAxis(glm::radians(90.0f),
                                        glm::vec3(1.0f, 0.0f, 0.0f)),
             .linearVelocity = velocity},
            {0.36f, 0.78f, 0.26f, 1.0f});
  }
}

void PhysicsTestScene::pickPhysicsBody(glm::vec2 screenPoint) {
  const glm::vec2 windowSize =
      windowSize_.x > 0.0f && windowSize_.y > 0.0f ? windowSize_
                                                    : camera_.mScreenSize;
  const glm::vec2 framebufferSize =
      framebufferSize_.x > 0.0f && framebufferSize_.y > 0.0f
          ? framebufferSize_
          : camera_.mScreenSize;
  if (windowSize.x <= 0.0f || windowSize.y <= 0.0f ||
      framebufferSize.x <= 0.0f || framebufferSize.y <= 0.0f) {
    return;
  }

  const glm::vec2 framebufferPoint{
      screenPoint.x * framebufferSize.x / windowSize.x,
      screenPoint.y * framebufferSize.y / windowSize.y};
  const glm::vec4 viewport{0.0f, 0.0f, framebufferSize.x, framebufferSize.y};
  const glm::vec3 nearPoint =
      glm::unProject({framebufferPoint.x,
                      framebufferSize.y - framebufferPoint.y, 0.0f},
                     camera_.getViewMatrix(), camera_.getPerspectiveTransform(),
                     viewport);
  const glm::vec3 farPoint =
      glm::unProject({framebufferPoint.x,
                      framebufferSize.y - framebufferPoint.y, 1.0f},
                     camera_.getViewMatrix(), camera_.getPerspectiveTransform(),
                     viewport);

  const glm::vec3 start = camera_.getPosition();
  const glm::vec3 direction = glm::normalize(farPoint - nearPoint);
  const glm::vec3 end = start + direction * 100.0f;

  pick_.hasRaycast = true;
  pick_.hit = physicsContext_.raycast(start, end);
  updatePickMarker();
}

void PhysicsTestScene::updatePickMarker() {
  markerLines_.clear();
  if (!pick_.hasRaycast) {
    return;
  }

  if (hitMarkerNode_ != nullptr) {
    hitMarkerNode_->setLocalPosition(
        pick_.hit.hasHit ? pick_.hit.point + pick_.hit.normal * 0.18f
                         : glm::vec3{0.0f, -1000.0f, 0.0f});
  }
}

void PhysicsTestScene::resetDynamicBodies() {
  ++resetCounter_;
  for (std::size_t i = 0; i < bodies_.size(); ++i) {
    auto &body = bodies_[i];
    if (body.type != DL::PhysicsBodyType::Dynamic) {
      continue;
    }
    const float offset =
        0.08f * static_cast<float>((static_cast<int>(i) + resetCounter_) % 5);
    physicsContext_.world().setBodyTransform(
        body.handle, body.spawnPosition + glm::vec3(offset, 0.25f, 0.0f),
        body.spawnRotation);
    physicsContext_.world().setLinearVelocity(body.handle, {0.0f, 0.0f, 0.0f});
    physicsContext_.world().setAwake(body.handle, true);
  }
  syncVisualBodies();
}

void PhysicsTestScene::updateCameraController(const DL::FrameContext &ctx) {
  if (ctx.input.isMouseButtonDown(DL::MouseButton::Right)) {
    cameraYaw_ -= ctx.input.mouseDelta.x * kCameraMouseSensitivity;
    cameraPitch_ =
        std::clamp(cameraPitch_ - ctx.input.mouseDelta.y * kCameraMouseSensitivity,
                   -kCameraMaxPitch, kCameraMaxPitch);
  }

  const glm::vec3 forward = cameraForward(cameraYaw_, cameraPitch_);
  camera_.lookAt(camera_.getPosition() + forward);

  glm::vec3 movement{0.0f};
  if (ctx.input.isActionDown(DL::Action::MoveForward)) {
    movement.z -= 1.0f;
  }
  if (ctx.input.isActionDown(DL::Action::MoveBackward)) {
    movement.z += 1.0f;
  }
  if (ctx.input.isActionDown(DL::Action::MoveRight)) {
    movement.x += 1.0f;
  }
  if (ctx.input.isActionDown(DL::Action::MoveLeft)) {
    movement.x -= 1.0f;
  }
  if (glm::length(movement) > 0.001f) {
    camera_.translate(glm::normalize(movement) * kCameraMoveSpeed *
                      static_cast<float>(ctx.delta_time));
  }
}

void PhysicsTestScene::render(const DL::FrameContext &ctx) {
#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Physics Test");
  ImGui::Text("Backend: Box3D");
  int staticBodies = 0;
  int kinematicBodies = 0;
  int dynamicBodies = 0;
  for (const auto &body : bodies_) {
    if (body.type == DL::PhysicsBodyType::Static) {
      ++staticBodies;
    } else if (body.type == DL::PhysicsBodyType::Kinematic) {
      ++kinematicBodies;
    } else if (body.type == DL::PhysicsBodyType::Dynamic) {
      ++dynamicBodies;
    }
  }
  ImGui::Text("Bodies: %d static, %d kinematic, %d dynamic", staticBodies,
              kinematicBodies, dynamicBodies);
  ImGui::Checkbox("Simulate", &simulationEnabled_);
  if (ImGui::Checkbox("Debug wire overlay", &debugLinesEnabled_)) {
    physicsContext_.setDebugRenderingEnabled(debugLinesEnabled_);
  }
  if (ImGui::Checkbox("Velocity lines", &velocityDebugEnabled_)) {
    physicsContext_.setDebugRenderSettings(
        {.collisionShapes = true,
         .velocityVectors = velocityDebugEnabled_,
         .velocityScale = 0.18f});
  }
  ImGui::Checkbox("Auto spawn", &autoSpawnEnabled_);
  if (ImGui::Button("Spawn pile")) {
    spawnBurst(10);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset dynamic bodies")) {
    resetDynamicBodies();
  }
  ImGui::Separator();
  if (!pick_.hasRaycast) {
    ImGui::Text("Pick: click a physics body");
  } else if (!pick_.hit.hasHit) {
    ImGui::Text("Pick: miss");
  } else {
    ImGui::Text("Pick: body %zu", pick_.hit.body.value);
    ImGui::Text("Point: %.2f %.2f %.2f", pick_.hit.point.x, pick_.hit.point.y,
                pick_.hit.point.z);
    ImGui::Text("Normal: %.2f %.2f %.2f", pick_.hit.normal.x,
                pick_.hit.normal.y, pick_.hit.normal.z);
  }
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void PhysicsTestScene::onScreenSizeChanged(glm::vec2 size) {
  windowSize_ = size;
  SceneNode::onScreenSizeChanged(size);
}

void PhysicsTestScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
  camera_.mScreenSize = size;
}
