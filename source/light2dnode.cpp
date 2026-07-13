#include "light2dnode.h"

#include "light2dvisualizer.h"
#include <algorithm>
#include <cmath>
#include <utility>

Light2DNode::Light2DNode(DL::IRenderDevice *renderDevice,
                         DL::RenderResourceCache *renderResourceCache,
                         DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache), camera_(camera),
      currentIntensity_(config_.intensity) {}

Light2DNode::Light2DNode(Config config, DL::IRenderDevice *renderDevice,
                         DL::RenderResourceCache *renderResourceCache,
                         DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), config_(config), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache), camera_(camera),
      currentIntensity_(config_.intensity) {}

void Light2DNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void Light2DNode::update(const DL::FrameContext &ctx) {
  if (config_.flickerAmount > 0.0f && config_.flickerSpeed > 0.0f) {
    const float waveA = std::sin(static_cast<float>(ctx.total_time) *
                                 config_.flickerSpeed);
    const float waveB = std::sin(static_cast<float>(ctx.total_time) *
                                     config_.flickerSpeed * 1.73f +
                                 1.91f);
    const float flicker = (waveA * 0.65f + waveB * 0.35f) *
                          std::clamp(config_.flickerAmount, 0.0f, 1.0f);
    currentIntensity_ = std::max(0.0f, config_.intensity * (1.0f + flicker));
  } else {
    currentIntensity_ = config_.intensity;
  }
  SceneNode::update(ctx);
}

void Light2DNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void Light2DNode::setRadius(float radius) {
  config_.radius = std::max(0.0f, radius);
}

void Light2DNode::setIntensity(float intensity) {
  config_.intensity = std::max(0.0f, intensity);
}

void Light2DNode::setSoftness(float softness) {
  config_.softness = std::clamp(softness, 0.0f, 1.0f);
}

void Light2DNode::setFlickerAmount(float amount) {
  config_.flickerAmount = std::clamp(amount, 0.0f, 1.0f);
}

void Light2DNode::setFlickerSpeed(float speed) {
  config_.flickerSpeed = std::max(0.0f, speed);
}

void Light2DNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 26.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void Light2DNode::initComponents() {
  if (camera_ == nullptr) {
    return;
  }
  auto visualizer = std::make_unique<DL::Light2DVisualizer>(
      *camera_, *this, renderDevice_, renderResourceCache_);
  addRenderComponent(std::move(visualizer));
}
