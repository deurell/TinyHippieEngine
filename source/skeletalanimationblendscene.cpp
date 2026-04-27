#include "skeletalanimationblendscene.h"

#include "debugui.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <string>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

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

  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 2.4f, 11.0f));
  camera_->lookAt(cameraTarget_);

  auto meshNode = std::make_unique<MeshNode>(
      "Resources/character-l.glb", codeBook_, renderDevice_, meshAssetCache_,
      renderResourceCache_, this, camera_.get());
  meshNode->init();
  meshNode->setDebugName("character-l");
  meshNode->setVisualizerSettings(visualizerSettings_);
  meshNode->setLocalPosition({walkDistance_, -2.2f, 0.0f});
  meshNode->setLocalScale({3.0f, 3.0f, 3.0f});
  meshNode_ = meshNode.get();
  addChild(std::move(meshNode));

  resolveAnimationClips();
  applyAnimationBlend();
}

void SkeletalAnimationBlendScene::update(const DL::FrameContext &ctx) {
  if (meshNode_ != nullptr) {
    if (autoCycle_) {
      const float cycleTime = std::fmod(static_cast<float>(ctx.total_time), 6.0f);
      walking_ = cycleTime >= 1.0f && cycleTime < 4.2f;
    }
    targetWalkBlendWeight_ = walking_ ? 1.0f : 0.0f;

    const float maxStep = blendRate_ * static_cast<float>(ctx.delta_time);
    if (walkBlendWeight_ < targetWalkBlendWeight_) {
      walkBlendWeight_ =
          std::min(walkBlendWeight_ + maxStep, targetWalkBlendWeight_);
    } else {
      walkBlendWeight_ =
          std::max(walkBlendWeight_ - maxStep, targetWalkBlendWeight_);
    }

    walkDistance_ += 1.35f * walkBlendWeight_ * static_cast<float>(ctx.delta_time);
    if (walkDistance_ > 2.6f) {
      walkDistance_ = -2.6f;
    }
    meshNode_->setLocalPosition({walkDistance_, -2.2f, 0.0f});
    applyAnimationBlend();
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
  ImGui::Checkbox("Auto walking/stopping", &autoCycle_);
  if (!autoCycle_) {
    ImGui::Checkbox("Walking", &walking_);
  }
  ImGui::Text("Blend idle -> walk: %.2f", walkBlendWeight_);
  if (meshNode_ != nullptr && meshNode_->hasAnimations()) {
    ImGui::Text("Idle clip: %s",
                std::string(meshNode_->animationClipName(idleClipIndex_)).c_str());
    ImGui::Text("Walk clip: %s",
                std::string(meshNode_->animationClipName(walkClipIndex_)).c_str());
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
}

void SkeletalAnimationBlendScene::applyAnimationBlend() {
  if (meshNode_ == nullptr || !meshNode_->hasAnimations()) {
    return;
  }
  meshNode_->setAnimationPlaying(true);
  meshNode_->setAnimationLooping(true);
  meshNode_->setAnimationBlend(idleClipIndex_, walkClipIndex_, walkBlendWeight_);
}
