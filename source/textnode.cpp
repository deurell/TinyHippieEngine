#include "textnode.h"
#include "textvisualizer.h"

TextNode::TextNode(std::string_view glslVersionString,
                   DL::SceneNode *parentNode, std::string text)
    : DL::SceneNode(parentNode), glslVersionString_(glslVersionString.data()),
      text_(text) {}

void TextNode::init() {
  initCamera();
  initComponents();
  SceneNode::init();
}

void TextNode::update(float delta) { SceneNode::update(delta); }

void TextNode::render(float delta) { SceneNode::render(delta); }

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
      "main", *camera_, glslVersionString_, *this, text_,
      "Resources/C64_Pro-STYLE.ttf", "Shaders/starwars.vert",
      "Shaders/starwars.frag");
  visualizers.emplace_back(std::move(component));
}
