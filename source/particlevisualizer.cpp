#include "particlevisualizer.h"

#include "particlesystemnode.h"
#include <glm/gtc/matrix_transform.hpp>

DL::ParticleVisualizer::ParticleVisualizer(
    std::string name, DL::Camera &camera, ParticleSystemNode &node,
    DL::IRenderDevice *renderDevice, std::string vertexShaderPath,
    std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      particleNode_(node), renderDevice_(renderDevice) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ =
      renderDevice_->createPipeline(vertexShaderPath_, fragmentShaderPath_);
  mesh_ = renderDevice_->createTexturedQuad();
}

DL::ParticleVisualizer::~ParticleVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid()) {
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

    const glm::mat4 localParticleTransform =
        glm::translate(glm::mat4(1.0f), particle.position) *
        glm::mat4_cast(particle.rotation) *
        glm::scale(glm::mat4(1.0f), particle.scale);

    DL::DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.blendMode = DL::BlendMode::Additive;
    const auto &config = particleNode_.getConfig();
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
