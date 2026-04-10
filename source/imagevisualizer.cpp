#include "imagevisualizer.h"

DL::ImageVisualizer::ImageVisualizer(
    std::string name, DL::Camera &camera, std::string_view glslVersionString,
    SceneNode &node, std::string texturePath,
    basist::etc1_global_selector_codebook *codeBook, DL::IRenderDevice *renderDevice,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), std::string(glslVersionString),
                     vertexShaderPath, fragmentShaderPath, node),
      renderDevice_(renderDevice), texturePath_(std::move(texturePath)),
      codeBook_(codeBook) {
  if (renderDevice_ != nullptr && codeBook_ != nullptr) {
    pipeline_ = renderDevice_->createPipeline(vertexShaderPath, fragmentShaderPath,
                                              glslVersionString);
    mesh_ = renderDevice_->createTexturedQuad();
    texture_ = renderDevice_->createBasisTexture(texturePath_, *codeBook_);
  }
}

DL::ImageVisualizer::~ImageVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (texture_.valid()) {
      renderDevice_->destroy(texture_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void DL::ImageVisualizer::render(const glm::mat4 &worldTransform,
                                 const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !texture_.valid() ||
      !pipeline_.valid()) {
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

  DL::DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.texture = texture_;
  command.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  command.uniforms.push_back(DL::UniformValue::makeMat4("view", view));
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("projection", projectionMatrix));
  renderDevice_->draw(command);
}
