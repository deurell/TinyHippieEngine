#include "textnode.h"
#include "textvisualizer.h"

#include <algorithm>
#include <glm/gtc/quaternion.hpp>

TextNode::TextNode(DL::SceneNode *parentNode, std::string text,
                   DL::IRenderDevice *renderDevice,
                   DL::RenderResourceCache *renderResourceCache,
                   DL::Camera *camera)
    : DL::SceneNode(parentNode), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache), text_(text),
      camera_(camera) {}

void TextNode::init() {
  initCamera();
  initComponents();
  SceneNode::init();
}

void TextNode::update(const DL::FrameContext &ctx) {
  updateBillboardRotation();
  SceneNode::update(ctx);
}

void TextNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void TextNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void TextNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 10));
  localCamera_->lookAt({0, 0, 0});
  camera_ = localCamera_.get();
}

void TextNode::initComponents() {
  auto component = std::make_unique<DL::TextVisualizer>(
      *camera_, *this, text_, "Resources/C64_Pro-STYLE.ttf",
      renderDevice_, renderResourceCache_, "Shaders/status.vert",
      "Shaders/status.frag", fontPixelHeight_);
  component->setAlignment(alignment_);
  component->setAnchor(anchor_);
  component->setTextColor(textColor_);
  component->setShadowColor(shadowColor_);
  component->setShadowOffset(shadowOffset_);
  textVisualizer_ = component.get();
  addRenderComponent(std::move(component));
}

void TextNode::updateBillboardRotation() {
  if (!billboardEnabled_ || camera_ == nullptr) {
    return;
  }

  setLocalRotation(glm::inverse(camera_->mOrientation));
}

void TextNode::setTextAlignment(DL::TextAlignment alignment) {
  alignment_ = alignment;
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setAlignment(alignment_);
  }
}

void TextNode::setTextAnchor(DL::TextAnchor anchor) {
  anchor_ = anchor;
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setAnchor(anchor_);
  }
}

void TextNode::setFontPixelHeight(float pixelHeight) {
  fontPixelHeight_ = std::max(pixelHeight, 1.0f);
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setFontPixelHeight(fontPixelHeight_);
  }
}

void TextNode::setTextColor(glm::vec4 color) {
  textColor_ = color;
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setTextColor(textColor_);
  }
}

void TextNode::setShadowColor(glm::vec4 color) {
  shadowColor_ = color;
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setShadowColor(shadowColor_);
  }
}

void TextNode::setShadowOffset(glm::vec2 offset) {
  shadowOffset_ = offset;
  if (textVisualizer_ != nullptr) {
    textVisualizer_->setShadowOffset(shadowOffset_);
  }
}
