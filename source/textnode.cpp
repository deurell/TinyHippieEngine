#include "textnode.h"
#include "textvisualizer.h"

TextNode::TextNode(DL::SceneNode *parentNode, std::string text,
                   DL::IRenderDevice *renderDevice)
    : DL::SceneNode(parentNode), renderDevice_(renderDevice), text_(text) {}

void TextNode::init() {
  initCamera();
  initComponents();
  SceneNode::init();
}

void TextNode::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

void TextNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void TextNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  camera_->mScreenSize = size;
  auto *visualizer =
      dynamic_cast<DL::TextVisualizer *>(getVisualizer("main"));
  if (visualizer != nullptr) {
    visualizer->setLayoutWidth(size.x);
  }
}

void TextNode::initCamera() {
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 10));
}

void TextNode::initComponents() {
  auto component = std::make_unique<DL::TextVisualizer>(
      "main", *camera_, *this, text_, "Resources/C64_Pro-STYLE.ttf",
      renderDevice_, "Shaders/starwars.vert",
      "Shaders/starwars.frag");
  visualizers.emplace_back(std::move(component));
}
