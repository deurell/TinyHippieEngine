#include "spritenode.h"

#include "spritevisualizer.h"
#include <utility>

SpriteNode::SpriteNode(std::string imagePath,
                       basist::etc1_global_selector_codebook *codeBook,
                       DL::IRenderDevice *renderDevice,
                       DL::SceneNode *parentNode, DL::Camera *camera)
    : SceneNode(parentNode), imagePath_(std::move(imagePath)),
      codeBook_(codeBook), camera_(camera), renderDevice_(renderDevice) {}

void SpriteNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void SpriteNode::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

void SpriteNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void SpriteNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void SpriteNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  localCamera_->lookAt({0, 0, 0});
  camera_ = localCamera_.get();
}

void SpriteNode::initComponents() {
  if (camera_ == nullptr) {
    return;
  }

  auto visualizer = std::make_unique<DL::SpriteVisualizer>(
      "SpriteVisualizer", *camera_, *this, imagePath_, codeBook_,
      renderDevice_);
  visualizers.emplace_back(std::move(visualizer));
}
