//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planerendercomponent.h"

PlaneNode::PlaneNode(DL::SceneNode *parentNode, DL::Camera *camera,
                     DL::IRenderDevice *renderDevice,
                     DL::RenderResourceCache *renderResourceCache)
    : DL::SceneNode(parentNode), camera_(camera), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

void PlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void PlaneNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);

  if (planeRenderComponent_ != nullptr) {
    planeRenderComponent_->baseColor = color;
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
  std::string vertexShaderPath = "Shaders/simple.vert";
  std::string fragmentShaderPath = "Shaders/simple.frag";
  if (planeType == PlaneType::Spinner) {
    vertexShaderPath = "Shaders/spinner.vert";
    fragmentShaderPath = "Shaders/spinner.frag";
  }

  auto renderer = std::make_unique<DL::PlaneRenderComponent>(
      *camera_, *this, renderDevice_, renderResourceCache_,
      vertexShaderPath, fragmentShaderPath);
  renderer->baseColor = color;
  renderer->spinnerEnabled = planeType == PlaneType::Spinner;
  planeRenderComponent_ = renderer.get();
  addRenderComponent(std::move(renderer));
}
