//
// Created by Mikael Deurell on 2023-08-16.
//

#include "simplenodescene.h"
#include "planecomponent.h"

SimpleNodeScene::SimpleNodeScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void SimpleNodeScene::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void SimpleNodeScene::render(float delta) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  SceneNode::render(delta);
}
void SimpleNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  mScreenSize = size;
  mCamera->mScreenSize = size;
}
void SimpleNodeScene::initCamera() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
}

void SimpleNodeScene::initComponents() {
  auto component = std::make_unique<DL::PlaneComponent>("custom", *mCamera, mGlslVersionString);
  components.emplace_back(std::move(component));
}
