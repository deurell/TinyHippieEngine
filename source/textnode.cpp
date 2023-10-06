#include "textnode.h"
#include "textvisualizer.h"

TextNode::TextNode(std::string_view glslVersionString,
                   DL::SceneNode *parentNode)
    : DL::SceneNode(parentNode), glslVersionString_(glslVersionString.data()) {}

void TextNode::init() {
    SceneNode::init();
    initCamera();
    initComponents();
    setLocalPosition({0, 0,0});
}

void TextNode::update(float delta) {SceneNode::update(delta);}

void TextNode::render(float delta) {SceneNode::render(delta);}

void TextNode::onScreenSizeChanged(glm::vec2 size) {
    SceneNode::onScreenSizeChanged(size);
    screenSize_ = size;
    camera_->mScreenSize = size;
}

void TextNode::initCamera() {
    camera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
    camera_->lookAt({0, 0, 0});
}

void TextNode::initComponents() {
    auto component = std::make_unique<DL::TextVisualizer>(
        "custom", *camera_, glslVersionString_, *this);
    visualizers.emplace_back(std::move(component));
}
