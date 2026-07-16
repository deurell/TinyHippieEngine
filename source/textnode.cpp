#include "textnode.h"
#include "textrendercomponent.h"

#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <utility>

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
  auto component = std::make_unique<DL::TextRenderComponent>(
      *camera_, *this, text_, "Resources/C64_Pro-STYLE.ttf",
      renderDevice_, renderResourceCache_, "Shaders/status.vert",
      "Shaders/status.frag", fontPixelHeight_);
  component->setAlignment(alignment_);
  component->setAnchor(anchor_);
  component->setTextColor(textColor_);
  component->setShadowColor(shadowColor_);
  component->setShadowOffset(shadowOffset_);
  textRenderComponent_ = component.get();
  addRenderComponent(std::move(component));
}

void TextNode::updateBillboardRotation() {
  if (!billboardEnabled_ || camera_ == nullptr) {
    return;
  }

  setLocalRotation(glm::inverse(camera_->mOrientation));
}

void TextNode::setText(std::string text) {
  text_ = std::move(text);
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setText(text_);
  }
}

void TextNode::setTextAlignment(DL::TextAlignment alignment) {
  alignment_ = alignment;
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setAlignment(alignment_);
  }
}

void TextNode::setTextAnchor(DL::TextAnchor anchor) {
  anchor_ = anchor;
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setAnchor(anchor_);
  }
}

void TextNode::setFontPixelHeight(float pixelHeight) {
  fontPixelHeight_ = std::max(pixelHeight, 1.0f);
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setFontPixelHeight(fontPixelHeight_);
  }
}

void TextNode::setTextColor(glm::vec4 color) {
  textColor_ = color;
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setTextColor(textColor_);
  }
}

void TextNode::setShadowColor(glm::vec4 color) {
  shadowColor_ = color;
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setShadowColor(shadowColor_);
  }
}

void TextNode::setShadowOffset(glm::vec2 offset) {
  shadowOffset_ = offset;
  if (textRenderComponent_ != nullptr) {
    textRenderComponent_->setShadowOffset(shadowOffset_);
  }
}
