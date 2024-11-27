//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planevisualizer.h"

PlaneNode::PlaneNode(std::string_view glslVersionString,
                     DL::SceneNode *parentNode, DL::Camera *camera)
    : glslVersionString_(glslVersionString.data()), DL::SceneNode(parentNode),
      camera_(camera) {}

void PlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void PlaneNode::update(float delta) {
  SceneNode::update(delta);

  auto* visualizer= dynamic_cast<DL::PlaneVisualizer*>(getVisualizer("PlaneVisualizer"));
  visualizer->baseColor = color;
}

void PlaneNode::render(float delta) { SceneNode::render(delta); }

void PlaneNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  camera_->mScreenSize = size;
}

void PlaneNode::initCamera() {
  if (camera_) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  localCamera_->lookAt({0, 0, 0});
  camera_ = localCamera_.get();
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
      "PlaneVisualizer", *camera_, glslVersionString_, *this, shaderModifier,
      vertexShaderPath, fragmentShaderPath);
  visualizer->baseColor = color;

  visualizers.emplace_back(std::move(visualizer));
}
