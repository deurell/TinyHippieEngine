//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planevisualizer.h"

PlaneNode::PlaneNode(std::string_view glslVersionString,
                     DL::SceneNode *parentNode)
    : DL::SceneNode(parentNode), mGlslVersionString(glslVersionString.data()) {}

void PlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void PlaneNode::update(float delta) { SceneNode::update(delta); }

void PlaneNode::render(float delta) { SceneNode::render(delta); }

void PlaneNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

void PlaneNode::initCamera() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
}

void PlaneNode::initComponents() {
  auto visualizer = std::make_unique<DL::PlaneVisualizer>(
      "custom", *mCamera, mGlslVersionString, *this);
  visualizers.emplace_back(std::move(visualizer));
}
