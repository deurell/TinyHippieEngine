//
// Created by Mikael Deurell on 2023-08-16.
//

#include "simplenode.h"
#include "planecomponent.h"

SimpleNode::SimpleNode(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void SimpleNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({5,0,0});
}

void SimpleNode::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  SceneNode::render(delta);
}
void SimpleNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  mScreenSize = size;
  mCamera->mScreenSize = size;
}
void SimpleNode::initCamera() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
}

void SimpleNode::initComponents() {
  auto component = std::make_unique<DL::PlaneComponent>("custom", *mCamera, mGlslVersionString);
  components.emplace_back(std::move(component));
}
