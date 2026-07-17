#include "planerendercomponent.h"

#include "renderqueue.h"
#include "scenenode.h"
#include <utility>

DL::PlaneRenderComponent::PlaneRenderComponent(
    DL::Camera &camera, SceneNode &node, DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : RenderComponent(camera, vertexShaderPath, fragmentShaderPath, node),
      renderDevice_(renderDevice), resourceCache_(resourceCache) {
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

DL::PlaneRenderComponent::~PlaneRenderComponent() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void DL::PlaneRenderComponent::render(const glm::mat4 &worldTransform,
                                 const DL::FrameContext &ctx,
                                 DL::RenderPassId pass) {
  if (pass != DL::RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);

  auto position = extractPosition(worldTransform);
  auto rotation = extractRotation(worldTransform);
  auto scale = extractScale(worldTransform);

  model = glm::translate(model, position);
  model = model * glm::mat4_cast(rotation);
  model = glm::scale(model, scale);

  glm::mat4 view = camera_.getViewMatrix();
  glm::mat4 projectionMatrix = camera_.getPerspectiveTransform();

  DL::RenderItem item;
  item.tag = DL::RenderTag::Opaque;
  item.renderLayer = node_.renderLayer();
  item.localBounds = {.center = glm::vec3(0.0f),
                      .halfExtents = glm::vec3(1.0f, 1.0f, 0.0f)};
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.pass = pass;
  if (proceduralStyle > 0) {
    item.blendMode =
        (proceduralStyle == 1 || proceduralStyle == 2 || proceduralStyle == 9)
            ? DL::BlendMode::Additive
            : DL::BlendMode::Alpha;
    item.depthTest = false;
  }
  item.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  item.uniforms.push_back(DL::UniformValue::makeVec4("baseColor", baseColor));
  item.uniforms.push_back(
      DL::UniformValue::makeInt("proceduralStyle", proceduralStyle));
  item.uniforms.push_back(
      DL::UniformValue::makeVec4("proceduralParams", proceduralParams));
  item.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  item.uniforms.push_back(DL::UniformValue::makeMat4("view", view));
  item.uniforms.push_back(
      DL::UniformValue::makeMat4("projection", projectionMatrix));
  if (spinnerEnabled) {
    item.uniforms.push_back(
        DL::UniformValue::makeFloat("speed", spinnerSpeed));
  }
  DL::submitRenderItem(ctx, *renderDevice_, std::move(item));
}
