//
// Created by Mikael Deurell on 2023-08-16.
//

#include "simplenodescene.h"

SimpleNodeScene::SimpleNodeScene(std::string_view glslVersionString)
    : mGlslVersionString(glslVersionString) {}

void SimpleNodeScene::init() {
  SceneNode::init();
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
  auto shader = std::make_unique<DL::Shader>(
      "Shaders/simple.vert", "Shaders/simple.frag", mGlslVersionString);

  mPlane = std::make_unique<DL::Plane>(std::move(shader), *mCamera);
  mPlane->position = {0, 0, 0};
  mPlane->scale = {16, 16, 1};
}

void SimpleNodeScene::render(float delta) {
  SceneNode::render(delta);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  mPlane->render(delta);

}
void SimpleNodeScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  mScreenSize = size;
  mCamera->mScreenSize = size;
}
