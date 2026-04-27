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

constexpr int kChaserCount = 10;
constexpr float kCharacterHalfWidth = 0.46f;
constexpr float kCharacterHalfDepth = 0.34f;
constexpr float kFollowerHalfWidth = 0.62f;
constexpr float kFollowerHalfDepth = 0.48f;
constexpr float kHeroWalkSpeed = 1.05f;
constexpr float kHeroRunSpeed = 2.1f;
constexpr float kFollowerWalkPlaybackPerSpeed = 0.025f;
constexpr float kFollowerRunPlaybackPerSpeed = 0.025f;
constexpr float kFollowerRunSlotDistance = 1.45f;
constexpr float kFollowerArriveDistance = 0.10f;
constexpr float kFollowerFacingDeadzone = 0.01f;
constexpr float kFollowerSeparationStrength = 0.35f;
constexpr glm::vec3 kCharacterScale{0.2f, 0.2f, 0.2f};
constexpr glm::vec3 kInitialFollowerFacing{0.0f, 0.0f, -1.0f};

glm::vec3 safeNormalize(const glm::vec3 &value) {
  const float length = glm::length(value);
  return length > 0.0001f ? value / length : glm::vec3(0.0f);
}

} // namespace

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
    std::string assetPath, std::string debugName, const glm::vec3 &position) {
  auto node = std::make_unique<MeshNode>(
      std::move(assetPath), codeBook_, renderDevice_, meshAssetCache_,
      renderResourceCache_, this, camera_.get());
  node->init();
  node->setDebugName(std::move(debugName));
  node->setVisualizerSettings(visualizerSettings_);
  node->setLocalPosition(position);
  node->setLocalScale(kCharacterScale);
  node->setAnimationPlaying(true);
  node->setAnimationLooping(true);
  return node;
}

void SkeletalAnimationBlendScene::initCamera() {
  camera_ = std::make_unique<DL::Camera>(glm::vec3(4.8f, 4.3f, 5.6f));
  camera_->mFov = 34.0f;
  camera_->lookAt(cameraTarget_);
}

void SkeletalAnimationBlendScene::initLeadCharacter() {
  auto meshNode =
      createCharacterNode("Resources/character-l.glb", "character-l",
                          characterPosition_);
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
        createCharacterNode("Resources/character-q.glb", "chaser",
                            spawnPosition);
    chaserNode->setAnimationPlaybackSpeed(0.0f);
    chaserNode->setAnimationBlend(idleClipIndex_, walkClipIndex_, 0.0f);

    chasers_.push_back({.node = chaserNode.get(),
                        .position = spawnPosition,
                        .preCorrectionPosition = spawnPosition,
                        .plannedMoveDelta = glm::vec3(0.0f),
                        .desiredMoveDirection = kInitialFollowerFacing,
                        .facingDirection = kInitialFollowerFacing,
                        .desiredState = AiState::Idle,
                        .animationBlend = 0.0f});
    addChild(std::move(chaserNode));
  }
}

void SkeletalAnimationBlendScene::update(const DL::FrameContext &ctx) {
  if (meshNode_ != nullptr) {
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
  if (meshNode_ == nullptr) {
    return;
  }

  for (std::size_t i = 0; i < chasers_.size(); ++i) {
    moveFollowerToSlot(chasers_[i], i, deltaTime);
  }

  for (auto &agent : chasers_) {
    agent.preCorrectionPosition = agent.position;
  }
  resolveFlockOverlaps();
  for (auto &agent : chasers_) {
    updateFollowerPresentation(agent, deltaTime);
  }
}

void SkeletalAnimationBlendScene::moveFollowerToSlot(FlockAgent &agent,
                                                     std::size_t index,
                                                     float deltaTime) {
  if (agent.node == nullptr) {
    return;
  }

  agent.plannedMoveDelta = glm::vec3(0.0f);
  const glm::vec3 toSlot = flockSlotPosition(index) - agent.position;
  const float slotDistance = glm::length(toSlot);
  if (slotDistance <= kFollowerArriveDistance) {
    agent.desiredState = AiState::Idle;
    return;
  }

  agent.desiredState =
      slotDistance > kFollowerRunSlotDistance ? AiState::Run : AiState::Walk;
  const float moveSpeed =
      agent.desiredState == AiState::Run ? kHeroRunSpeed : kHeroWalkSpeed;
  const glm::vec3 direction = toSlot / slotDistance;
  const float step = std::min(slotDistance, moveSpeed * deltaTime);
  agent.desiredMoveDirection = direction;
  agent.plannedMoveDelta = direction * step;
  agent.position += agent.plannedMoveDelta;
}

void SkeletalAnimationBlendScene::updateFollowerPresentation(FlockAgent &agent,
                                                             float deltaTime) {
  if (agent.node == nullptr) {
    return;
  }

  const float locomotionSpeed =
      deltaTime > 0.0f ? glm::length(glm::vec3(agent.plannedMoveDelta.x, 0.0f,
                                               agent.plannedMoveDelta.z)) /
                             deltaTime
                       : 0.0f;
  const bool moving = locomotionSpeed > 0.03f;
  const bool running = moving && agent.desiredState == AiState::Run;

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
  const float blendStep = blendRate_ * deltaTime;
  if (agent.animationBlend < targetBlend) {
    agent.animationBlend = std::min(agent.animationBlend + blendStep, targetBlend);
  } else {
    agent.animationBlend = std::max(agent.animationBlend - blendStep, targetBlend);
  }

  const float playbackScale =
      running ? kFollowerRunPlaybackPerSpeed : kFollowerWalkPlaybackPerSpeed;
  const float playbackSpeed =
      std::clamp(locomotionSpeed * playbackScale, 0.0f, 0.055f);
  agent.node->setAnimationPlaybackSpeed(playbackSpeed);
  agent.node->setAnimationBlend(idleClipIndex_,
                                running ? sprintClipIndex_ : walkClipIndex_,
                                agent.animationBlend);
  agent.node->setLocalPosition(agent.position);
}

glm::vec3 SkeletalAnimationBlendScene::flockSlotPosition(std::size_t index) const {
  const glm::vec3 forward = safeNormalize(leaderForward_);
  const glm::vec3 right = safeNormalize(glm::vec3(forward.z, 0.0f, -forward.x));
  const int row = static_cast<int>(index / 5u);
  const int col = static_cast<int>(index % 5u);
  const float sideOffset = (static_cast<float>(col) - 2.0f) * 0.75f;
  const float backOffset = 1.25f + static_cast<float>(row) * 0.72f;
  return characterPosition_ - forward * backOffset + right * sideOffset;
}

void SkeletalAnimationBlendScene::resolveFlockOverlaps() {
  for (auto &agent : chasers_) {
    const glm::vec3 push =
        separationFromCharacterBounds(agent.position, characterPosition_);
    agent.position += push;
  }

  for (std::size_t i = 0; i < chasers_.size(); ++i) {
    for (std::size_t j = i + 1; j < chasers_.size(); ++j) {
      const glm::vec3 push =
          separationFromBounds(chasers_[i].position, chasers_[j].position,
                               kFollowerHalfWidth, kFollowerHalfDepth) *
          kFollowerSeparationStrength;
      chasers_[i].position += push * 0.5f;
      chasers_[j].position -= push * 0.5f;
    }
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
