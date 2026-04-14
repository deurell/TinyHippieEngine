#include "textnode.h"
#include "textvisualizer.h"

TextNode::TextNode(DL::SceneNode *parentNode, std::string text,
                   DL::IRenderDevice *renderDevice,
                   DL::RenderResourceCache *renderResourceCache)
    : DL::SceneNode(parentNode), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache), text_(text) {}

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
}

void TextNode::initCamera() {
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 10));
}

void TextNode::initComponents() {
  auto component = std::make_unique<DL::TextVisualizer>(
      *camera_, *this, text_, "Resources/C64_Pro-STYLE.ttf",
      renderDevice_, renderResourceCache_, "Shaders/starwars.vert",
      "Shaders/starwars.frag");
  textVisualizer_ = component.get();
  addRenderComponent(std::move(component));
}
