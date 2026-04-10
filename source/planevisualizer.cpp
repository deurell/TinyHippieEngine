#include "planevisualizer.h"

DL::PlaneVisualizer::PlaneVisualizer(
    std::string name, DL::Camera &camera, SceneNode &node,
    DL::IRenderDevice *renderDevice,
    std::string vertexShaderPath, std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), vertexShaderPath,
                     fragmentShaderPath, node),
      renderDevice_(renderDevice) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ =
      renderDevice_->createPipeline(vertexShaderPath_, fragmentShaderPath_);
  mesh_ = renderDevice_->createTexturedQuad();
}

DL::PlaneVisualizer::~PlaneVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
}

void DL::PlaneVisualizer::render(const glm::mat4 &worldTransform,
                                 const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !pipeline_.valid()) {
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
  command.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(DL::UniformValue::makeVec4("baseColor", baseColor));
  command.uniforms.push_back(DL::UniformValue::makeMat4("model", model));
  command.uniforms.push_back(DL::UniformValue::makeMat4("view", view));
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("projection", projectionMatrix));
  if (spinnerEnabled) {
    command.uniforms.push_back(
        DL::UniformValue::makeFloat("speed", spinnerSpeed));
  }
  renderDevice_->draw(command);
}
