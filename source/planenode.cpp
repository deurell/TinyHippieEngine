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
  std::string vertexShaderPath = planeType == PlaneType::Simple
                                     ? "Shaders/simple.vert"
                                     : "Shaders/spinner.vert";
  std::string fragmentShaderPath = planeType == PlaneType::Simple
                                       ? "Shaders/simple.frag"
                                       : "Shaders/spinner.frag";
  auto shaderModifier =
      planeType == PlaneType::Spinner
          ? [](DL::Shader &shader) { shader.setFloat("speed", 0.25f); }
          : nullptr;

  auto visualizer = std::make_unique<DL::PlaneVisualizer>(
      "PlaneVisualizer", *mCamera,
      mGlslVersionString, *this, shaderModifier, vertexShaderPath,
      fragmentShaderPath);

  visualizers.emplace_back(std::move(visualizer));
}
