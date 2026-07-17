//
// Created by Mikael Deurell on 2023-08-16.
//

#include "planenode.h"
#include "planerendercomponent.h"

namespace {

bool isDeflektorPlane(PlaneNode::PlaneType planeType) {
  return planeType >= PlaneNode::PlaneType::DeflektorBeam;
}

int proceduralStyle(PlaneNode::PlaneType planeType) {
  switch (planeType) {
  case PlaneNode::PlaneType::DeflektorBeam:
    return 1;
  case PlaneNode::PlaneType::DeflektorSource:
    return 2;
  case PlaneNode::PlaneType::DeflektorTarget:
    return 3;
  case PlaneNode::PlaneType::DeflektorBlocker:
    return 4;
  case PlaneNode::PlaneType::DeflektorReflectiveBlock:
    return 5;
  case PlaneNode::PlaneType::DeflektorReflectorManual:
    return 6;
  case PlaneNode::PlaneType::DeflektorReflectorAuto:
    return 7;
  case PlaneNode::PlaneType::DeflektorSelection:
    return 8;
  case PlaneNode::PlaneType::DeflektorExplosion:
    return 9;
  case PlaneNode::PlaneType::Simple:
  case PlaneNode::PlaneType::Spinner:
    return 0;
  }
  return 0;
}

} // namespace

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
    planeRenderComponent_->proceduralParams = proceduralParams;
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
  } else if (isDeflektorPlane(planeType)) {
    fragmentShaderPath = "Shaders/deflektorish.frag";
  }

  auto renderer = std::make_unique<DL::PlaneRenderComponent>(
      *camera_, *this, renderDevice_, renderResourceCache_,
      vertexShaderPath, fragmentShaderPath);
  renderer->baseColor = color;
  renderer->spinnerEnabled = planeType == PlaneType::Spinner;
  renderer->proceduralStyle = proceduralStyle(planeType);
  planeRenderComponent_ = renderer.get();
  addRenderComponent(std::move(renderer));
}
