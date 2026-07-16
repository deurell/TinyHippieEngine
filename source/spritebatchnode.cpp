#include "spritebatchnode.h"

#include <utility>

SpriteBatchNode::SpriteBatchNode(DL::SpriteBatchConfig config,
                                 DL::IRenderDevice *renderDevice,
                                 DL::RenderResourceCache *renderResourceCache,
                                 DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), config_(std::move(config)), camera_(camera),
      renderDevice_(renderDevice), renderResourceCache_(renderResourceCache) {}

void SpriteBatchNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void SpriteBatchNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
}

void SpriteBatchNode::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void SpriteBatchNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void SpriteBatchNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void SpriteBatchNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }

  addRenderComponent(std::make_unique<DL::SpriteBatchRenderComponent>(
      *camera_, *this, config_, renderDevice_, renderResourceCache_));
}
