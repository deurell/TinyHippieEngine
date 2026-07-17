#include "shaderplanenode.h"

#include <utility>

namespace DL {

ShaderPlaneNode::ShaderPlaneNode(SceneNode *parentNode, Camera *camera,
                                 IRenderDevice *renderDevice,
                                 RenderResourceCache *renderResourceCache)
    : SceneNode(parentNode), camera_(camera), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

ShaderPlaneNode::ShaderPlaneNode(Config nodeConfig, SceneNode *parentNode,
                                 Camera *camera, IRenderDevice *renderDevice,
                                 RenderResourceCache *renderResourceCache)
    : SceneNode(parentNode), config(std::move(nodeConfig)), camera_(camera),
      renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache) {}

void ShaderPlaneNode::init() {
  SceneNode::init();
  initCamera();
  initComponent();
}

void ShaderPlaneNode::update(const FrameContext &ctx) {
  SceneNode::update(ctx);

  if (planeRenderComponent_ == nullptr) {
    return;
  }
  planeRenderComponent_->baseColor = config.color;
  planeRenderComponent_->proceduralStyle = config.proceduralStyle;
  planeRenderComponent_->proceduralParams = config.params0;
  planeRenderComponent_->proceduralParams2 = config.params1;
  planeRenderComponent_->blendMode = config.blendMode;
  planeRenderComponent_->depthTest = config.depthTest;
}

void ShaderPlaneNode::render(const FrameContext &ctx) { SceneNode::render(ctx); }

void ShaderPlaneNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void ShaderPlaneNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 26.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void ShaderPlaneNode::initComponent() {
  auto renderer = std::make_unique<PlaneRenderComponent>(
      *camera_, *this, renderDevice_, renderResourceCache_, config.vertexShader,
      config.fragmentShader);
  renderer->baseColor = config.color;
  renderer->proceduralStyle = config.proceduralStyle;
  renderer->proceduralParams = config.params0;
  renderer->proceduralParams2 = config.params1;
  renderer->blendMode = config.blendMode;
  renderer->depthTest = config.depthTest;
  planeRenderComponent_ = renderer.get();
  addRenderComponent(std::move(renderer));
}

} // namespace DL
