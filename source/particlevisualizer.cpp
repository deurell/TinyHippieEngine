#include "particlevisualizer.h"

#include "particlesystemnode.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

DL::ParticleVisualizer::ParticleVisualizer(
    DL::Camera &camera, ParticleSystemNode &node, DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      particleNode_(node), renderDevice_(renderDevice),
      resourceCache_(resourceCache),
      billboardEnabled_(particleNode_.isBillboardEnabled()) {
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

DL::ParticleVisualizer::~ParticleVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

glm::mat4 DL::ParticleVisualizer::buildBillboardModel(
    const glm::vec3 &worldPosition, const glm::vec3 &scale,
    const DL::Camera &camera) {
  glm::vec3 forward = camera.getPosition() - worldPosition;
  if (glm::length(forward) < 0.0001f) {
    forward = glm::vec3(glm::inverse(glm::mat4_cast(camera.mOrientation)) *
                        glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
  }
  forward = glm::normalize(forward);

  glm::vec3 up{0.0f, 1.0f, 0.0f};
  if (std::abs(glm::dot(forward, up)) > 0.98f) {
    up = glm::vec3{1.0f, 0.0f, 0.0f};
  }

  const glm::vec3 right = glm::normalize(glm::cross(up, forward));
  up = glm::normalize(glm::cross(forward, right));

  glm::mat4 model(1.0f);
  model[0] = glm::vec4(right * scale.x, 0.0f);
  model[1] = glm::vec4(up * scale.y, 0.0f);
  model[2] = glm::vec4(forward * scale.z, 0.0f);
  model[3] = glm::vec4(worldPosition, 1.0f);
  return model;
}

void DL::ParticleVisualizer::render(const glm::mat4 &worldTransform,
                                    const DL::FrameContext &ctx,
                                    DL::RenderPassId pass) {
  if (pass != DL::RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  for (const auto &particle : particleNode_.getParticles()) {
    if (!particle.alive) {
      continue;
    }

    const auto &config = particleNode_.getConfig();
    glm::vec3 renderScale = particle.scale;
    const float speed = glm::length(particle.velocity);
    if (config.render.stretchByVelocity > 0.0f && speed > 0.001f) {
      const float stretch =
          std::min(1.0f + speed * config.render.stretchByVelocity,
                   config.render.maxStretch);
      renderScale.y *= stretch;
    }

    const glm::vec3 worldPosition =
        glm::vec3(worldTransform * glm::vec4(particle.position, 1.0f));
    const glm::vec3 nodeScale = extractScale(worldTransform);
    glm::mat4 particleTransform =
        glm::translate(glm::mat4(1.0f), worldPosition);
    if (billboardEnabled_) {
      particleTransform =
          buildBillboardModel(worldPosition, nodeScale * renderScale, camera_);
    } else {
      particleTransform = particleTransform *
                          glm::mat4_cast(particle.rotation) *
                          glm::scale(glm::mat4(1.0f), nodeScale * renderScale);
    }

    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.pass = pass;
    command.blendMode = config.render.blendMode;
    command.sortMode = DrawSortMode::BackToFront;
    command.sortDepth = cameraDistanceSortDepth(worldPosition);
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "iTime", static_cast<float>(ctx.total_time)));
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "paletteSteps", config.render.paletteSteps));
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "coreRadius", config.render.coreRadius));
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "haloRadius", config.render.haloRadius));
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "outerRadius", config.render.outerRadius));
    command.uniforms.push_back(DL::UniformValue::makeFloat(
        "sparkleAmount", config.render.sparkle));
    command.uniforms.push_back(
        DL::UniformValue::makeVec4("hotColor", config.render.hotColor));
    command.uniforms.push_back(
        DL::UniformValue::makeVec4("deepColor", config.render.deepColor));
    command.uniforms.push_back(
        DL::UniformValue::makeVec4("baseColor", particle.color));
    command.uniforms.push_back(
        DL::UniformValue::makeVec3("particleScale", nodeScale * renderScale));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("model", particleTransform));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
    command.uniforms.push_back(DL::UniformValue::makeMat4(
        "projection", camera_.getPerspectiveTransform()));
    renderDevice_->draw(command);
  }
}
