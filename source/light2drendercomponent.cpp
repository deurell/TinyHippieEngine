#include "light2drendercomponent.h"

#include "light2dnode.h"
#include "renderqueue.h"
#include "scenenode.h"
#include <glm/gtc/quaternion.hpp>
#include <utility>

DL::Light2DRenderComponent::Light2DRenderComponent(
    Camera &camera, Light2DNode &node, IRenderDevice *renderDevice,
    RenderResourceCache *resourceCache, std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : RenderComponent(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      lightNode_(node), renderDevice_(renderDevice), resourceCache_(resourceCache) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);
  mesh_ = resourceCache_ != nullptr ? resourceCache_->acquireTexturedQuad()
                                    : renderDevice_->createTexturedQuad();
}

DL::Light2DRenderComponent::~Light2DRenderComponent() {
  if (renderDevice_ == nullptr) {
    return;
  }
  if (mesh_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(mesh_);
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(pipeline_);
  }
}

void DL::Light2DRenderComponent::render(const glm::mat4 &worldTransform,
                                   const FrameContext &ctx,
                                   RenderPassId pass) {
  if (pass != RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  const auto &config = lightNode_.config();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform) *
                                glm::vec3(config.radius));

  RenderItem item;
  item.tag = RenderTag::Overlay;
  item.renderLayer = node_.renderLayer();
  item.localBounds = {.center = glm::vec3(0.0f),
                      .halfExtents = glm::vec3(1.0f, 1.0f, 0.0f)};
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.pass = pass;
  item.blendMode = BlendMode::Additive;
  item.depthTest = false;
  item.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  item.uniforms.push_back(UniformValue::makeVec4("lightColor",
                                                 config.color));
  item.uniforms.push_back(
      UniformValue::makeFloat("intensity", lightNode_.currentIntensity()));
  item.uniforms.push_back(
      UniformValue::makeFloat("softness", config.softness));
  item.uniforms.push_back(UniformValue::makeMat4("model", model));
  item.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  item.uniforms.push_back(UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  submitRenderItem(ctx, *renderDevice_, std::move(item));
}
