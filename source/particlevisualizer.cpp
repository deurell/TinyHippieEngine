#include "particlevisualizer.h"

#include "particlesystemnode.h"
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

DL::ParticleVisualizer::ParticleVisualizer(
    DL::Camera &camera, ParticleSystemNode &node, DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      particleNode_(node), renderDevice_(renderDevice),
      resourceCache_(resourceCache) {
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

void DL::ParticleVisualizer::render(const glm::mat4 &worldTransform,
                                    const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  for (const auto &particle : particleNode_.getParticles()) {
    if (!particle.alive) {
      continue;
    }

    const auto &config = particleNode_.getConfig();
    glm::vec3 renderScale = particle.scale;
    glm::quat renderRotation = particle.rotation;
    const float speed = glm::length(particle.velocity);
    if (config.render.stretchByVelocity > 0.0f && speed > 0.001f) {
      const float stretch =
          std::min(1.0f + speed * config.render.stretchByVelocity,
                   config.render.maxStretch);
      renderScale.y *= stretch;
      const glm::vec3 velocityDir = glm::normalize(particle.velocity);
      const float angle = std::atan2(velocityDir.y, velocityDir.x) -
                          glm::half_pi<float>();
      renderRotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    const glm::mat4 localParticleTransform =
        glm::translate(glm::mat4(1.0f), particle.position) *
        glm::mat4_cast(renderRotation) *
        glm::scale(glm::mat4(1.0f), renderScale);

    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.blendMode = config.render.blendMode;
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
        DL::UniformValue::makeMat4("model", worldTransform * localParticleTransform));
    command.uniforms.push_back(
        DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
    command.uniforms.push_back(DL::UniformValue::makeMat4(
        "projection", camera_.getPerspectiveTransform()));
    renderDevice_->draw(command);
  }
}
