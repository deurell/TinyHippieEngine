#include "skeletalanimationblendscene.h"

#include "debugui.h"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace {

constexpr int kChaserCount = 15;
constexpr float kCharacterHalfWidth = 0.46f;
constexpr float kCharacterHalfDepth = 0.34f;
constexpr float kFollowerHalfWidth = 0.62f;
constexpr float kFollowerHalfDepth = 0.48f;
constexpr float kHeroWalkSpeed = 1.05f;
constexpr float kHeroRunSpeed = 2.1f;
constexpr float kCameraMoveSpeed = 4.2f;
constexpr float kCameraMouseSensitivity = 0.0035f;
constexpr float kCameraMaxPitch = 1.2f;
constexpr float kFollowerIdlePlaybackSpeed = 0.55f;
constexpr float kFollowerWalkPlaybackPerSpeed = 0.005f;
constexpr float kFollowerRunPlaybackPerSpeed = 0.01f;
constexpr float kFollowerWalkEnterDistance = 0.18f;
constexpr float kFollowerWalkExitDistance = 0.10f;
constexpr float kFollowerRunEnterDistance = 1.75f;
constexpr float kFollowerRunExitDistance = 1.35f;
constexpr float kFollowerStateHoldSeconds = 0.16f;
constexpr float kFollowerArriveDistance = 0.10f;
constexpr float kFollowerFacingDeadzone = 0.01f;
constexpr float kFollowerFleeTriggerDistance = 1.15f;
constexpr float kFollowerFleeSeconds = 0.44f;
constexpr float kFollowerFleeSpeed = 2.65f;
constexpr float kFollowerRegroupSeconds = 2.2f;
constexpr float kFollowerSeparationStrength = 0.35f;
constexpr float kLeaderSeparationMaxStep = 0.035f;
constexpr float kFollowerCollisionPauseSeconds = 0.22f;
constexpr float kFollowerLongCollisionPauseSeconds = 0.48f;
constexpr glm::vec3 kCharacterScale{0.2f, 0.2f, 0.2f};
constexpr glm::vec3 kHeroScale{0.5f, 0.5f, 0.5f};
constexpr glm::vec3 kInitialFollowerFacing{0.0f, 0.0f, -1.0f};
constexpr DL::Key kCameraForwardKey = DL::Key::W;
constexpr DL::Key kCameraBackwardKey = DL::Key::S;
constexpr DL::Key kCameraLeftKey = DL::Key::A;
constexpr DL::Key kCameraRightKey = DL::Key::D;
constexpr int kMoveClipSearchIterations = 6;

glm::vec3 safeNormalize(const glm::vec3 &value) {
  const float length = glm::length(value);
  return length > 0.0001f ? value / length : glm::vec3(0.0f);
}

glm::vec3 limitLength(const glm::vec3 &value, float maxLength) {
  const float length = glm::length(value);
  if (length <= maxLength || length <= 0.0001f) {
    return value;
  }
  return value * (maxLength / length);
}

glm::vec3 cameraForward(float yaw, float pitch) {
  const float cosPitch = std::cos(pitch);
  return glm::normalize(
      glm::vec3(std::sin(yaw) * cosPitch, std::sin(pitch),
                std::cos(yaw) * cosPitch));
}

} // namespace

struct SkeletalAnimationBlendScene::FlockBehavior {
  static void advance(SkeletalAnimationBlendScene &scene, float deltaTime);

private:
  static void beginFrame(SkeletalAnimationBlendScene &scene);
  static void planFollowerMotion(SkeletalAnimationBlendScene &scene,
                                 FlockAgent &agent, std::size_t index,
                                 float deltaTime);
  [[nodiscard]] static bool moveFollowerAwayFromLeader(
      SkeletalAnimationBlendScene &scene, FlockAgent &agent, std::size_t index,
      float deltaTime);
  [[nodiscard]] static bool holdFollowerIdle(FlockAgent &agent, float deltaTime);
  static void chaseFollowerSlot(SkeletalAnimationBlendScene &scene,
                                FlockAgent &agent, std::size_t index,
                                float deltaTime);
  static void updateFollowerPresentation(SkeletalAnimationBlendScene &scene,
                                         FlockAgent &agent, float deltaTime);
  static void triggerFollowerCollisionPause(FlockAgent &agent, float seconds);
  [[nodiscard]] static AiState chooseFollowerState(FlockAgent &agent,
                                                   float slotDistance);
  static void resolveFlockOverlaps(SkeletalAnimationBlendScene &scene);
  [[nodiscard]] static float collisionPauseSeconds(
      const SkeletalAnimationBlendScene &scene, std::size_t index);
  [[nodiscard]] static glm::vec3 flockSlotPosition(
      const SkeletalAnimationBlendScene &scene, std::size_t index);
  [[nodiscard]] static bool isStaticIdle(const FlockAgent &agent);
  [[nodiscard]] static bool overlapsLeader(const SkeletalAnimationBlendScene &scene,
                                           const glm::vec3 &position);
  [[nodiscard]] static glm::vec3 moveWithoutEnteringLeader(
      const SkeletalAnimationBlendScene &scene, const glm::vec3 &position,
      const glm::vec3 &moveDelta);
  static void applyFollowerSeparation(FlockAgent &first, FlockAgent &second,
                                      const glm::vec3 &push);
};

SkeletalAnimationBlendScene::SkeletalAnimationBlendScene(
    DL::IRenderDevice *renderDevice,
    basist::etc1_global_selector_codebook *codeBook,
    DL::MeshAssetCache *meshAssetCache,
    DL::RenderResourceCache *renderResourceCache)
    : SceneNode(nullptr), renderDevice_(renderDevice), codeBook_(codeBook),
      meshAssetCache_(meshAssetCache),
      renderResourceCache_(renderResourceCache) {
  visualizerSettings_.lightDirection =
      glm::normalize(glm::vec3(0.25f, 1.0f, 0.55f));
  visualizerSettings_.ambientStrength = 0.58f;
  visualizerSettings_.specularStrength = 0.08f;
}

void SkeletalAnimationBlendScene::init() {
  SceneNode::init();

  initCamera();
  initLeadCharacter();
  resolveAnimationClips();
  updateLeadAnimationBlend(0.0f);
  initFollowers();
}

std::unique_ptr<MeshNode> SkeletalAnimationBlendScene::createCharacterNode(
    std::string assetPath, std::string debugName, const glm::vec3 &position,
    const glm::vec3 &scale) {
  auto node = std::make_unique<MeshNode>(
      std::move(assetPath), codeBook_, renderDevice_, meshAssetCache_,
      renderResourceCache_, this, camera_.get());
  node->init();
  node->setDebugName(std::move(debugName));
  node->setVisualizerSettings(visualizerSettings_);
  node->setLocalPosition(position);
  node->setLocalScale(scale);
  node->setAnimationPlaying(true);
  node->setAnimationLooping(true);
  return node;
}

void SkeletalAnimationBlendScene::initCamera() {
  camera_ = std::make_unique<DL::Camera>(glm::vec3(4.8f, 4.3f, 5.6f));
  camera_->mFov = 34.0f;
  camera_->lookAt(cameraTarget_);
  const glm::vec3 toTarget = glm::normalize(cameraTarget_ - camera_->getPosition());
  cameraYaw_ = std::atan2(toTarget.x, toTarget.z);
  cameraPitch_ = std::asin(std::clamp(toTarget.y, -1.0f, 1.0f));
}

void SkeletalAnimationBlendScene::initLeadCharacter() {
  auto meshNode =
      createCharacterNode("Resources/character-l.glb", "character-l",
                          characterPosition_, kHeroScale);
  meshNode_ = meshNode.get();
  addChild(std::move(meshNode));
}

void SkeletalAnimationBlendScene::initFollowers() {
  chasers_.clear();
  chasers_.reserve(kChaserCount);
  for (int i = 0; i < kChaserCount; ++i) {
    const float row = static_cast<float>(i / 5);
    const float col = static_cast<float>(i % 5);
    const glm::vec3 spawnPosition{-4.8f + col * 0.85f, 0.0f,
                                  2.55f + row * 0.8f};
    auto chaserNode =
        createCharacterNode("Resources/character-r.glb", "chaser",
                            spawnPosition, kCharacterScale);
    chaserNode->setAnimationPlaybackSpeed(0.0f);
    chaserNode->setAnimationBlend(idleClipIndex_, walkClipIndex_, 0.0f);

    chasers_.push_back({.node = chaserNode.get(),
                        .position = spawnPosition,
                        .frameStartPosition = spawnPosition,
                        .preCorrectionPosition = spawnPosition,
                        .plannedMoveDelta = glm::vec3(0.0f),
                        .facingDirection = kInitialFollowerFacing,
                        .desiredState = AiState::Idle,
                        .stateHoldRemaining = 0.0f,
                        .collisionPauseRemaining = 0.0f,
                        .fleeRemaining = 0.0f,
                        .regroupRemaining = 0.0f,
                        .animationBlend = 0.0f,
                        .wasTouchingLeader = false});
    addChild(std::move(chaserNode));
  }
}

void SkeletalAnimationBlendScene::update(const DL::FrameContext &ctx) {
  if (meshNode_ != nullptr) {
    updateCameraController(ctx);
    if (aiEnabled_) {
      advanceAi(static_cast<float>(ctx.delta_time));
    }
    advanceFlock(static_cast<float>(ctx.delta_time));
    updateLeadAnimationBlend(static_cast<float>(ctx.delta_time));
  }

  SceneNode::update(ctx);
}

void SkeletalAnimationBlendScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.78f, 0.84f, 0.80f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  ImGui::Begin("Skeletal Animation Blend");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Checkbox("Simple AI", &aiEnabled_);
  ImGui::Text("State: %s", std::string(aiStateName()).c_str());
  ImGui::Text("Chasers: %d", static_cast<int>(chasers_.size()));
  ImGui::Text("Blend idle -> locomotion: %.2f", locomotionBlendWeight_);
  if (!aiEnabled_) {
    if (ImGui::Button("Idle")) {
      aiState_ = AiState::Idle;
    }
    ImGui::SameLine();
    if (ImGui::Button("Walk")) {
      aiState_ = AiState::Walk;
      locomotionClipIndex_ = walkClipIndex_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Run")) {
      aiState_ = AiState::Run;
      locomotionClipIndex_ = sprintClipIndex_;
    }
  }
  if (meshNode_ != nullptr && meshNode_->hasAnimations()) {
    ImGui::Text("Idle clip: %s",
                std::string(meshNode_->animationClipName(idleClipIndex_)).c_str());
    ImGui::Text("Walk clip: %s",
                std::string(meshNode_->animationClipName(walkClipIndex_)).c_str());
    ImGui::Text("Run clip: %s",
                std::string(meshNode_->animationClipName(sprintClipIndex_)).c_str());
  }
  if (ImGui::Checkbox("Debug normals", &debugNormals_) && meshNode_ != nullptr) {
    meshNode_->setDebugNormals(debugNormals_);
  }
  bool changed = false;
  changed |= ImGui::SliderFloat3("Light dir", &visualizerSettings_.lightDirection.x,
                                 -1.0f, 1.0f);
  changed |= ImGui::ColorEdit3("Light color", &visualizerSettings_.lightColor.x);
  changed |= ImGui::SliderFloat("Ambient", &visualizerSettings_.ambientStrength,
                                0.0f, 1.5f);
  changed |= ImGui::SliderFloat("Specular", &visualizerSettings_.specularStrength,
                                0.0f, 1.0f);
  if (changed && meshNode_ != nullptr) {
    meshNode_->setVisualizerSettings(visualizerSettings_);
  }
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void SkeletalAnimationBlendScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
}

std::size_t SkeletalAnimationBlendScene::findClipIndex(std::string_view name,
                                                       std::size_t fallback) const {
  if (meshNode_ == nullptr) {
    return fallback;
  }
  for (std::size_t index = 0; index < meshNode_->animationClipCount(); ++index) {
    if (meshNode_->animationClipName(index) == name) {
      return index;
    }
  }
  return fallback < meshNode_->animationClipCount() ? fallback : 0u;
}

void SkeletalAnimationBlendScene::resolveAnimationClips() {
  idleClipIndex_ = findClipIndex("idle", 0u);
  walkClipIndex_ = findClipIndex("walk", idleClipIndex_);
  sprintClipIndex_ = findClipIndex("sprint", walkClipIndex_);
  locomotionClipIndex_ = walkClipIndex_;
}

void SkeletalAnimationBlendScene::updateCameraController(
    const DL::FrameContext &ctx) {
  if (camera_ == nullptr) {
    return;
  }

  if (ctx.input.isMouseButtonDown(DL::MouseButton::Right)) {
    cameraYaw_ -= ctx.input.mouseDelta.x * kCameraMouseSensitivity;
    cameraPitch_ =
        std::clamp(cameraPitch_ - ctx.input.mouseDelta.y * kCameraMouseSensitivity,
                   -kCameraMaxPitch, kCameraMaxPitch);
  }

  const glm::vec3 forward = cameraForward(cameraYaw_, cameraPitch_);
  camera_->lookAt(camera_->getPosition() + forward);

  const float movementStep =
      kCameraMoveSpeed * static_cast<float>(ctx.delta_time);
  if (ctx.input.isKeyDown(kCameraForwardKey)) {
    camera_->translate(0.0f, 0.0f, -movementStep);
  }
  if (ctx.input.isKeyDown(kCameraBackwardKey)) {
    camera_->translate(0.0f, 0.0f, movementStep);
  }
  if (ctx.input.isKeyDown(kCameraRightKey)) {
    camera_->translate(movementStep, 0.0f, 0.0f);
  }
  if (ctx.input.isKeyDown(kCameraLeftKey)) {
    camera_->translate(-movementStep, 0.0f, 0.0f);
  }
}

void SkeletalAnimationBlendScene::updateLeadAnimationBlend(float deltaTime) {
  if (meshNode_ == nullptr || !meshNode_->hasAnimations()) {
    return;
  }

  targetLocomotionBlendWeight_ = aiState_ == AiState::Idle ? 0.0f : 1.0f;
  const float maxStep = blendRate_ * deltaTime;
  if (locomotionBlendWeight_ < targetLocomotionBlendWeight_) {
    locomotionBlendWeight_ =
        std::min(locomotionBlendWeight_ + maxStep, targetLocomotionBlendWeight_);
  } else {
    locomotionBlendWeight_ =
        std::max(locomotionBlendWeight_ - maxStep, targetLocomotionBlendWeight_);
  }

  meshNode_->setAnimationPlaying(true);
  meshNode_->setAnimationLooping(true);
  const float playbackSpeed =
      aiState_ == AiState::Run ? 0.36f : aiState_ == AiState::Walk ? 0.24f : 0.55f;
  meshNode_->setAnimationPlaybackSpeed(playbackSpeed);
  meshNode_->setAnimationBlend(idleClipIndex_, locomotionClipIndex_,
                               locomotionBlendWeight_);
}

void SkeletalAnimationBlendScene::advanceAi(float deltaTime) {
  if (aiState_ == AiState::Idle) {
    aiStateTimeRemaining_ -= deltaTime;
    if (aiStateTimeRemaining_ <= 0.0f) {
      chooseNextMoveState();
    }
    meshNode_->setLocalPosition(characterPosition_);
    return;
  }

  const glm::vec3 target = currentTarget();
  const glm::vec3 toTarget = target - characterPosition_;
  const float distance = glm::length(toTarget);
  if (distance < 0.08f) {
    characterPosition_ = target;
    aiState_ = AiState::Idle;
    aiStateTimeRemaining_ = aiTargetIndex_ % 2 == 0 ? 0.9f : 1.35f;
    aiTargetIndex_ = (aiTargetIndex_ + 1u) % 4u;
    meshNode_->setLocalPosition(characterPosition_);
    return;
  }

  const glm::vec3 direction = toTarget / distance;
  leaderForward_ = direction;
  const float step = std::min(distance, currentMoveSpeed() * deltaTime);
  characterPosition_ += direction * step;
  meshNode_->setLocalPosition(characterPosition_);

  const float yaw = std::atan2(direction.x, direction.z);
  meshNode_->setLocalRotation(glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void SkeletalAnimationBlendScene::advanceFlock(float deltaTime) {
  SkeletalAnimationBlendScene::FlockBehavior::advance(*this, deltaTime);
}

void SkeletalAnimationBlendScene::FlockBehavior::advance(
    SkeletalAnimationBlendScene &scene, float deltaTime) {
  if (scene.meshNode_ == nullptr) {
    return;
  }

  beginFrame(scene);

  for (std::size_t i = 0; i < scene.chasers_.size(); ++i) {
    planFollowerMotion(scene, scene.chasers_[i], i, deltaTime);
  }

  for (auto &agent : scene.chasers_) {
    agent.preCorrectionPosition = agent.position;
  }
  resolveFlockOverlaps(scene);
  for (auto &agent : scene.chasers_) {
    updateFollowerPresentation(scene, agent, deltaTime);
  }
}

void SkeletalAnimationBlendScene::FlockBehavior::beginFrame(
    SkeletalAnimationBlendScene &scene) {
  for (auto &agent : scene.chasers_) {
    agent.frameStartPosition = agent.position;
  }
}

void SkeletalAnimationBlendScene::FlockBehavior::planFollowerMotion(
    SkeletalAnimationBlendScene &scene, SkeletalAnimationBlendScene::FlockAgent &agent,
    std::size_t index, float deltaTime) {
  if (agent.node == nullptr) {
    return;
  }

  if (agent.stateHoldRemaining > 0.0f) {
    agent.stateHoldRemaining =
        std::max(0.0f, agent.stateHoldRemaining - deltaTime);
  }

  if (moveFollowerAwayFromLeader(scene, agent, index, deltaTime)) {
    return;
  }

  if (holdFollowerIdle(agent, deltaTime)) {
    return;
  }

  chaseFollowerSlot(scene, agent, index, deltaTime);
}

bool SkeletalAnimationBlendScene::FlockBehavior::holdFollowerIdle(
    SkeletalAnimationBlendScene::FlockAgent &agent, float deltaTime) {
  bool shouldHold = false;
  if (agent.regroupRemaining > 0.0f) {
    agent.regroupRemaining =
        std::max(0.0f, agent.regroupRemaining - deltaTime);
    shouldHold = true;
  }
  if (agent.collisionPauseRemaining > 0.0f) {
    agent.collisionPauseRemaining =
        std::max(0.0f, agent.collisionPauseRemaining - deltaTime);
    shouldHold = true;
  }

  if (!shouldHold) {
    return false;
  }

  agent.desiredState = SkeletalAnimationBlendScene::AiState::Idle;
  agent.plannedMoveDelta = glm::vec3(0.0f);
  if (agent.node != nullptr) {
    agent.node->setLocalPosition(agent.position);
  }
  return true;
}

void SkeletalAnimationBlendScene::FlockBehavior::chaseFollowerSlot(
    SkeletalAnimationBlendScene &scene, SkeletalAnimationBlendScene::FlockAgent &agent,
    std::size_t index, float deltaTime) {
  agent.plannedMoveDelta = glm::vec3(0.0f);
  const glm::vec3 toSlot = flockSlotPosition(scene, index) - agent.position;
  const float slotDistance = glm::length(toSlot);
  if (slotDistance <= kFollowerArriveDistance) {
    agent.desiredState = SkeletalAnimationBlendScene::AiState::Idle;
    agent.stateHoldRemaining = 0.0f;
    return;
  }

  agent.desiredState = chooseFollowerState(agent, slotDistance);
  const float moveSpeed =
      agent.desiredState == SkeletalAnimationBlendScene::AiState::Run
          ? kHeroRunSpeed
          : kHeroWalkSpeed;
  const glm::vec3 direction = toSlot / slotDistance;
  const float step = std::min(slotDistance, moveSpeed * deltaTime);
  agent.plannedMoveDelta =
      moveWithoutEnteringLeader(scene, agent.position, direction * step);
  if (glm::length(agent.plannedMoveDelta) <= 0.0001f) {
    triggerFollowerCollisionPause(agent, collisionPauseSeconds(scene, index));
    return;
  }
  agent.position += agent.plannedMoveDelta;
}

bool SkeletalAnimationBlendScene::FlockBehavior::moveFollowerAwayFromLeader(
    SkeletalAnimationBlendScene &scene, SkeletalAnimationBlendScene::FlockAgent &agent,
    std::size_t index, float deltaTime) {
  const glm::vec3 fromLeader = agent.position - scene.characterPosition_;
  const glm::vec3 flatFromLeader{fromLeader.x, 0.0f, fromLeader.z};
  const float leaderDistance = glm::length(flatFromLeader);
  if (leaderDistance < kFollowerFleeTriggerDistance) {
    agent.fleeRemaining = kFollowerFleeSeconds;
  } else if (agent.fleeRemaining > 0.0f) {
    const float previousFleeRemaining = agent.fleeRemaining;
    agent.fleeRemaining = std::max(0.0f, agent.fleeRemaining - deltaTime);
    if (previousFleeRemaining > 0.0f && agent.fleeRemaining <= 0.0f) {
      agent.regroupRemaining = kFollowerRegroupSeconds;
    }
  }

  if (agent.fleeRemaining <= 0.0f) {
    return false;
  }

  const glm::vec3 away =
      leaderDistance > 0.0001f ? flatFromLeader / leaderDistance
                               : -safeNormalize(scene.leaderForward_);
  const glm::vec3 leaderRight =
      safeNormalize(glm::vec3(scene.leaderForward_.z, 0.0f,
                              -scene.leaderForward_.x));
  const float sideSign = index % 2u == 0u ? 1.0f : -1.0f;
  const glm::vec3 fleeDirection =
      safeNormalize(away + leaderRight * sideSign * 0.35f);

  agent.desiredState = SkeletalAnimationBlendScene::AiState::Run;
  agent.collisionPauseRemaining = 0.0f;
  agent.plannedMoveDelta = fleeDirection * kFollowerFleeSpeed * deltaTime;
  agent.position += agent.plannedMoveDelta;
  return true;
}

void SkeletalAnimationBlendScene::FlockBehavior::updateFollowerPresentation(
    SkeletalAnimationBlendScene &scene, SkeletalAnimationBlendScene::FlockAgent &agent,
    float deltaTime) {
  if (agent.node == nullptr) {
    return;
  }

  if (agent.collisionPauseRemaining > 0.0f) {
    agent.desiredState = SkeletalAnimationBlendScene::AiState::Idle;
    agent.animationBlend =
        std::max(0.0f, agent.animationBlend - scene.blendRate_ * deltaTime);
    agent.node->setAnimationPlaybackSpeed(kFollowerIdlePlaybackSpeed);
    agent.node->setAnimationBlend(scene.idleClipIndex_, scene.walkClipIndex_,
                                  agent.animationBlend);
    agent.node->setLocalPosition(agent.position);
    return;
  }

  const glm::vec3 actualMoveDelta = agent.position - agent.frameStartPosition;
  const float actualSpeed =
      deltaTime > 0.0f
          ? glm::length(glm::vec3(actualMoveDelta.x, 0.0f, actualMoveDelta.z)) /
                deltaTime
          : 0.0f;
  const float plannedSpeed =
      deltaTime > 0.0f ? glm::length(glm::vec3(agent.plannedMoveDelta.x, 0.0f,
                                               agent.plannedMoveDelta.z)) /
                             deltaTime
                       : 0.0f;
  const bool moving = plannedSpeed > 0.03f && actualSpeed > 0.05f;
  const bool running = moving &&
      agent.desiredState == SkeletalAnimationBlendScene::AiState::Run;

  if (moving) {
    const glm::vec3 correctionDelta = agent.position - agent.preCorrectionPosition;
    const glm::vec3 finalMoveDelta = agent.plannedMoveDelta + correctionDelta;
    const glm::vec3 flatFinalMove{finalMoveDelta.x, 0.0f, finalMoveDelta.z};
    const glm::vec3 flatPlannedMove{agent.plannedMoveDelta.x, 0.0f,
                                    agent.plannedMoveDelta.z};
    const glm::vec3 facingDelta =
        glm::length(flatFinalMove) > kFollowerFacingDeadzone ? flatFinalMove
                                                             : flatPlannedMove;
    agent.facingDirection = safeNormalize(facingDelta);
    const float yaw = std::atan2(agent.facingDirection.x,
                                 agent.facingDirection.z);
    agent.node->setLocalRotation(
        glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)));
  }

  const float targetBlend = moving ? 1.0f : 0.0f;
  const float blendStep = scene.blendRate_ * deltaTime;
  if (agent.animationBlend < targetBlend) {
    agent.animationBlend =
        std::min(agent.animationBlend + blendStep, targetBlend);
  } else {
    agent.animationBlend =
        std::max(agent.animationBlend - blendStep, targetBlend);
  }

  const float playbackScale =
      running ? kFollowerRunPlaybackPerSpeed : kFollowerWalkPlaybackPerSpeed;
  const float playbackSpeed =
      moving ? std::clamp(actualSpeed * playbackScale, 0.0f, 0.055f)
             : kFollowerIdlePlaybackSpeed;
  agent.node->setAnimationPlaybackSpeed(playbackSpeed);
  agent.node->setAnimationBlend(scene.idleClipIndex_,
                                running ? scene.sprintClipIndex_
                                        : scene.walkClipIndex_,
                                agent.animationBlend);
  agent.node->setLocalPosition(agent.position);
}

void SkeletalAnimationBlendScene::FlockBehavior::triggerFollowerCollisionPause(
    SkeletalAnimationBlendScene::FlockAgent &agent, float seconds) {
  agent.collisionPauseRemaining =
      std::max(agent.collisionPauseRemaining, seconds);
  agent.desiredState = SkeletalAnimationBlendScene::AiState::Idle;
  agent.animationBlend = 0.0f;
}

SkeletalAnimationBlendScene::AiState
SkeletalAnimationBlendScene::FlockBehavior::chooseFollowerState(
    SkeletalAnimationBlendScene::FlockAgent &agent, float slotDistance) {
  if (agent.stateHoldRemaining > 0.0f) {
    return agent.desiredState;
  }

  const auto previousState = agent.desiredState;
  auto nextState = previousState;

  switch (previousState) {
  case SkeletalAnimationBlendScene::AiState::Idle:
    if (slotDistance > kFollowerRunEnterDistance) {
      nextState = SkeletalAnimationBlendScene::AiState::Run;
    } else if (slotDistance > kFollowerWalkEnterDistance) {
      nextState = SkeletalAnimationBlendScene::AiState::Walk;
    }
    break;
  case SkeletalAnimationBlendScene::AiState::Walk:
    if (slotDistance > kFollowerRunEnterDistance) {
      nextState = SkeletalAnimationBlendScene::AiState::Run;
    } else if (slotDistance <= kFollowerWalkExitDistance) {
      nextState = SkeletalAnimationBlendScene::AiState::Idle;
    }
    break;
  case SkeletalAnimationBlendScene::AiState::Run:
    if (slotDistance <= kFollowerRunExitDistance) {
      nextState = slotDistance > kFollowerWalkEnterDistance
                      ? SkeletalAnimationBlendScene::AiState::Walk
                      : SkeletalAnimationBlendScene::AiState::Idle;
    }
    break;
  }

  if (nextState != previousState) {
    agent.stateHoldRemaining = kFollowerStateHoldSeconds;
  }

  return nextState;
}

bool SkeletalAnimationBlendScene::FlockBehavior::isStaticIdle(
    const FlockAgent &agent) {
  return agent.desiredState == AiState::Idle &&
         glm::length(agent.plannedMoveDelta) <= 0.0001f;
}

bool SkeletalAnimationBlendScene::FlockBehavior::overlapsLeader(
    const SkeletalAnimationBlendScene &scene, const glm::vec3 &position) {
  return glm::length(
             scene.separationFromCharacterBounds(position, scene.characterPosition_)) >
         0.0f;
}

glm::vec3 SkeletalAnimationBlendScene::FlockBehavior::moveWithoutEnteringLeader(
    const SkeletalAnimationBlendScene &scene, const glm::vec3 &position,
    const glm::vec3 &moveDelta) {
  if (!overlapsLeader(scene, position + moveDelta)) {
    return moveDelta;
  }

  if (overlapsLeader(scene, position)) {
    return glm::vec3(0.0f);
  }

  float low = 0.0f;
  float high = 1.0f;
  for (int i = 0; i < kMoveClipSearchIterations; ++i) {
    const float mid = (low + high) * 0.5f;
    if (overlapsLeader(scene, position + moveDelta * mid)) {
      high = mid;
    } else {
      low = mid;
    }
  }
  return moveDelta * low;
}

void SkeletalAnimationBlendScene::FlockBehavior::applyFollowerSeparation(
    FlockAgent &first, FlockAgent &second, const glm::vec3 &push) {
  if (glm::length(push) <= 0.0f) {
    return;
  }

  const bool firstSettled = isStaticIdle(first);
  const bool secondSettled = isStaticIdle(second);
  if (firstSettled && secondSettled) {
    return;
  }

  if (firstSettled) {
    second.position -= push;
  } else if (secondSettled) {
    first.position += push;
  } else {
    first.position += push * 0.5f;
    second.position -= push * 0.5f;
  }
}

float SkeletalAnimationBlendScene::FlockBehavior::collisionPauseSeconds(
    const SkeletalAnimationBlendScene &scene, std::size_t index) {
  return (index + scene.aiTargetIndex_) % 4u == 0u
             ? kFollowerLongCollisionPauseSeconds
             : kFollowerCollisionPauseSeconds;
}

glm::vec3 SkeletalAnimationBlendScene::FlockBehavior::flockSlotPosition(
    const SkeletalAnimationBlendScene &scene, std::size_t index) {
  const glm::vec3 forward = safeNormalize(scene.leaderForward_);
  const glm::vec3 right = safeNormalize(glm::vec3(forward.z, 0.0f, -forward.x));
  const int row = static_cast<int>(index / 5u);
  const int col = static_cast<int>(index % 5u);
  const float sideOffset = (static_cast<float>(col) - 2.0f) * 0.75f;
  const float backOffset = 1.25f + static_cast<float>(row) * 0.72f;
  return scene.characterPosition_ - forward * backOffset + right * sideOffset;
}

void SkeletalAnimationBlendScene::FlockBehavior::resolveFlockOverlaps(
    SkeletalAnimationBlendScene &scene) {
  std::vector<bool> touchingLeader(scene.chasers_.size(), false);

  for (auto &agent : scene.chasers_) {
    const glm::vec3 push =
        scene.separationFromCharacterBounds(agent.position, scene.characterPosition_);
    if (glm::length(push) > 0.0f && !isStaticIdle(agent)) {
      agent.position += limitLength(push, kLeaderSeparationMaxStep);
      const std::size_t agentIndex =
          static_cast<std::size_t>(&agent - scene.chasers_.data());
      touchingLeader[agentIndex] = true;
      if (!agent.wasTouchingLeader) {
        triggerFollowerCollisionPause(agent, collisionPauseSeconds(scene, agentIndex));
      }
    }
  }

  for (std::size_t i = 0; i < scene.chasers_.size(); ++i) {
    for (std::size_t j = i + 1; j < scene.chasers_.size(); ++j) {
      const glm::vec3 push =
          scene.separationFromBounds(scene.chasers_[i].position,
                                     scene.chasers_[j].position, kFollowerHalfWidth,
                                     kFollowerHalfDepth) *
          kFollowerSeparationStrength;
      applyFollowerSeparation(scene.chasers_[i], scene.chasers_[j], push);
    }
  }

  for (std::size_t i = 0; i < scene.chasers_.size(); ++i) {
    scene.chasers_[i].wasTouchingLeader = touchingLeader[i];
  }
}

void SkeletalAnimationBlendScene::chooseNextMoveState() {
  const bool runLeg = aiTargetIndex_ == 1u || aiTargetIndex_ == 3u;
  aiState_ = runLeg ? AiState::Run : AiState::Walk;
  locomotionClipIndex_ = runLeg ? sprintClipIndex_ : walkClipIndex_;
}

glm::vec3 SkeletalAnimationBlendScene::separationFromCharacterBounds(
    const glm::vec3 &position, const glm::vec3 &otherPosition) const {
  return separationFromBounds(position, otherPosition, kCharacterHalfWidth,
                              kCharacterHalfDepth);
}

glm::vec3 SkeletalAnimationBlendScene::separationFromBounds(
    const glm::vec3 &position, const glm::vec3 &otherPosition,
    float halfWidth, float halfDepth) const {
  const float dx = position.x - otherPosition.x;
  const float dz = position.z - otherPosition.z;
  const float overlapX = halfWidth * 2.0f - std::abs(dx);
  const float overlapZ = halfDepth * 2.0f - std::abs(dz);
  if (overlapX <= 0.0f || overlapZ <= 0.0f) {
    return glm::vec3(0.0f);
  }

  if (overlapX < overlapZ) {
    return {dx >= 0.0f ? overlapX : -overlapX, 0.0f, 0.0f};
  }
  return {0.0f, 0.0f, dz >= 0.0f ? overlapZ : -overlapZ};
}

glm::vec3 SkeletalAnimationBlendScene::currentTarget() const {
  return aiTargets_[aiTargetIndex_ % 4u];
}

float SkeletalAnimationBlendScene::currentMoveSpeed() const {
  return aiState_ == AiState::Run ? kHeroRunSpeed : kHeroWalkSpeed;
}

std::string_view SkeletalAnimationBlendScene::aiStateName() const {
  switch (aiState_) {
  case AiState::Idle:
    return "Idle";
  case AiState::Walk:
    return "Walk";
  case AiState::Run:
    return "Run";
  }
  return "Unknown";
}
