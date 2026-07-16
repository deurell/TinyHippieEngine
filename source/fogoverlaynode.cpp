#include "fogoverlaynode.h"

#include "fogoverlayrendercomponent.h"
#include <algorithm>
#include <utility>

FogOverlayNode::FogOverlayNode(Config config, DL::IRenderDevice *renderDevice,
                               DL::RenderResourceCache *renderResourceCache,
                               DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), config_(std::move(config)), camera_(camera),
      renderDevice_(renderDevice), renderResourceCache_(renderResourceCache) {}

void FogOverlayNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void FogOverlayNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
}

void FogOverlayNode::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void FogOverlayNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void FogOverlayNode::setAlpha(float alpha) {
  config_.alpha = std::clamp(alpha, 0.0f, 1.0f);
}

void FogOverlayNode::setSoftness(float softness) {
  config_.softness = std::clamp(softness, 0.0f, 1.0f);
}

void FogOverlayNode::setTiling(glm::vec2 tiling) {
  config_.tiling = glm::max(tiling, glm::vec2(0.001f));
}

void FogOverlayNode::setSecondLayerStrength(float strength) {
  config_.secondLayerStrength = std::clamp(strength, 0.0f, 1.0f);
}

void FogOverlayNode::setPulseAmount(float amount) {
  config_.pulseAmount = std::clamp(amount, 0.0f, 1.0f);
}

void FogOverlayNode::setPulseSpeed(float speed) {
  config_.pulseSpeed = std::max(0.0f, speed);
}

void FogOverlayNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void FogOverlayNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }

  addRenderComponent(std::make_unique<DL::FogOverlayRenderComponent>(
      *camera_, *this, renderDevice_, renderResourceCache_));
}
