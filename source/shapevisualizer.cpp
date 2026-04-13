#include "shapevisualizer.h"

namespace DL {

ShapeVisualizer::ShapeVisualizer(std::string name, Camera &camera,
                                 SceneNode &node, GeneratedMeshData meshData,
                                 IRenderDevice *renderDevice,
                                 RenderResourceCache *resourceCache,
                                 std::string vertexShaderPath,
                                 std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(name), std::move(vertexShaderPath),
                     std::move(fragmentShaderPath), node),
      renderDevice_(renderDevice), resourceCache_(resourceCache) {
  if (renderDevice_ == nullptr) {
    return;
  }
  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);
  mesh_ = renderDevice_->createMesh(meshData.positions, meshData.normals,
                                    meshData.uvs, meshData.indices);
}

ShapeVisualizer::~ShapeVisualizer() {
  if (renderDevice_ == nullptr) {
    return;
  }
  if (mesh_.valid()) {
    renderDevice_->destroy(mesh_);
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_->destroy(pipeline_);
  }
}

void ShapeVisualizer::render(const glm::mat4 &worldTransform,
                             const FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(
      UniformValue::makeMat4("projection", camera_.getPerspectiveTransform()));
  command.uniforms.push_back(
      UniformValue::makeVec3("viewPos", camera_.getPosition()));
  command.uniforms.push_back(
      UniformValue::makeVec3("lightDirection", glm::normalize(lightDirection)));
  command.uniforms.push_back(
      UniformValue::makeVec3("lightColor", lightColor));
  command.uniforms.push_back(
      UniformValue::makeVec3("materialDiffuse", material.diffuse));
  command.uniforms.push_back(
      UniformValue::makeVec3("materialAmbient", material.ambient));
  command.uniforms.push_back(
      UniformValue::makeVec3("materialSpecular", material.specular));
  command.uniforms.push_back(
      UniformValue::makeFloat("materialShininess", material.shininess));
  renderDevice_->draw(command);
}

} // namespace DL
