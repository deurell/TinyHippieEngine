#include "light2dvisualizer.h"

#include "light2dnode.h"
#include <glm/gtc/quaternion.hpp>

DL::Light2DVisualizer::Light2DVisualizer(
    Camera &camera, Light2DNode &node, IRenderDevice *renderDevice,
    RenderResourceCache *resourceCache, std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
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

DL::Light2DVisualizer::~Light2DVisualizer() {
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

void DL::Light2DVisualizer::render(const glm::mat4 &worldTransform,
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

  DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.pass = pass;
  command.blendMode = BlendMode::Additive;
  command.depthTest = false;
  command.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(UniformValue::makeVec4("lightColor",
                                                    config.color));
  command.uniforms.push_back(
      UniformValue::makeFloat("intensity", lightNode_.currentIntensity()));
  command.uniforms.push_back(
      UniformValue::makeFloat("softness", config.softness));
  command.uniforms.push_back(UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  renderDevice_->draw(command);
}
