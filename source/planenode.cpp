//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planevisualizer.h"

PlaneNode::PlaneNode(DL::SceneNode *parentNode, DL::Camera *camera,
                     DL::IRenderDevice *renderDevice,
                     DL::RenderResourceCache *renderResourceCache)
    : DL::SceneNode(parentNode), camera_(camera), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

void PlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void PlaneNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);

  if (planeVisualizer_ != nullptr) {
    planeVisualizer_->baseColor = color;
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

  auto visualizer = std::make_unique<DL::PlaneVisualizer>(
      *camera_, *this, renderDevice_, renderResourceCache_,
      vertexShaderPath, fragmentShaderPath);
  visualizer->baseColor = color;
  visualizer->spinnerEnabled = planeType == PlaneType::Spinner;
  planeVisualizer_ = visualizer.get();
  addRenderComponent(std::move(visualizer));
}
