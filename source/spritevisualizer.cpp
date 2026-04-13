#include "spritevisualizer.h"

#include <iostream>
#include <utility>

DL::SpriteVisualizer::SpriteVisualizer(
    std::string name, DL::Camera &camera, SceneNode &node,
    std::string texturePath,
    basist::etc1_global_selector_codebook *codeBook,
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *resourceCache,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), vertexShaderPath,
                     fragmentShaderPath, node),
      renderDevice_(renderDevice), texturePath_(std::move(texturePath)),
      codeBook_(codeBook), resourceCache_(resourceCache) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath,
                                                  fragmentShaderPath);
  mesh_ = resourceCache_ != nullptr ? resourceCache_->acquireTexturedQuad()
                                    : renderDevice_->createTexturedQuad();
  loadTexture();
}

DL::SpriteVisualizer::~SpriteVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(mesh_);
    }
    if (texture_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(texture_);
    }
    if (pipeline_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void DL::SpriteVisualizer::render(const glm::mat4 &worldTransform,
                                  const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !texture_.valid() ||
      !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  DL::DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.texture = texture_;
  command.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  renderDevice_->draw(command);
}

bool DL::SpriteVisualizer::loadTexture() {
  if (codeBook_ == nullptr) {
    std::cerr << "Sprite texture needs a Basis codebook: " << texturePath_
              << "\n";
    return false;
  }

  texture_ = resourceCache_ != nullptr
                 ? resourceCache_->acquireBasisTexture(texturePath_, *codeBook_)
                 : renderDevice_->createBasisTexture(texturePath_, *codeBook_);
  if (!texture_.valid()) {
    std::cerr << "Failed to load sprite texture: " << texturePath_ << "\n";
    return false;
  }
  return true;
}
