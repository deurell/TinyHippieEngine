//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planevisualizer.h"

PlaneNode::PlaneNode(std::string_view glslVersionString,
                     DL::SceneNode *parentNode, DL::Camera *camera,
                     DL::IRenderDevice *renderDevice)
    : glslVersionString_(glslVersionString.data()), DL::SceneNode(parentNode),
      camera_(camera), renderDevice_(renderDevice) {}

void PlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void PlaneNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);

  auto *visualizer =
      dynamic_cast<DL::PlaneVisualizer *>(getVisualizer("PlaneVisualizer"));
  if (visualizer != nullptr) {
    visualizer->baseColor = color;
  }
}

void PlaneNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

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

  std::function<void(std::vector<DL::UniformValue> &)> uniformModifier;
  if (planeType == PlaneType::Spinner) {
    uniformModifier = [](std::vector<DL::UniformValue> &uniforms) {
      uniforms.push_back(DL::UniformValue::makeFloat("speed", 0.25f));
    };
  }

  auto visualizer = std::make_unique<DL::PlaneVisualizer>(
      "PlaneVisualizer", *camera_, glslVersionString_, *this, renderDevice_,
      uniformModifier, vertexShaderPath, fragmentShaderPath);
  visualizer->baseColor = color;

  visualizers.emplace_back(std::move(visualizer));
}
