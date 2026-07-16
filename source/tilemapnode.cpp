#include "tilemapnode.h"

#include <utility>

TileMapNode::TileMapNode(DL::TileMapConfig config,
                         DL::IRenderDevice *renderDevice,
                         DL::RenderResourceCache *renderResourceCache,
                         DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), config_(std::move(config)), camera_(camera),
      renderDevice_(renderDevice), renderResourceCache_(renderResourceCache) {}

void TileMapNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void TileMapNode::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

void TileMapNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void TileMapNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void TileMapNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void TileMapNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }

  addRenderComponent(std::make_unique<DL::TileMapRenderComponent>(
      *camera_, *this, config_, renderDevice_, renderResourceCache_));
}
