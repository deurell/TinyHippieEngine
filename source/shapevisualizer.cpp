#include "shapevisualizer.h"

#include "scenenode.h"
#include <limits>
#include <utility>

namespace DL {

namespace {

Bounds boundsFromPositions(const std::vector<glm::vec3> &positions) {
  if (positions.empty()) {
    return {};
  }

  glm::vec3 minPosition(std::numeric_limits<float>::max());
  glm::vec3 maxPosition(std::numeric_limits<float>::lowest());
  for (const glm::vec3 &position : positions) {
    minPosition = glm::min(minPosition, position);
    maxPosition = glm::max(maxPosition, position);
  }

  return {.center = (minPosition + maxPosition) * 0.5f,
          .halfExtents = (maxPosition - minPosition) * 0.5f};
}

} // namespace

ShapeVisualizer::ShapeVisualizer(Camera &camera, SceneNode &node,
                                 GeneratedMeshData meshData,
                                 IRenderDevice *renderDevice,
                                 RenderResourceCache *resourceCache,
                                 std::string vertexShaderPath,
                                 std::string fragmentShaderPath)
    : VisualizerBase(camera, std::move(vertexShaderPath),
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
  localBounds_ = boundsFromPositions(meshData.positions);
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
                             const FrameContext &ctx, RenderPassId pass) {
  if (pass != RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, extractPosition(worldTransform));
  model = model * glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  RenderItem item;
  item.tag = RenderTag::Opaque;
  item.renderLayer = node_.renderLayer();
  item.localBounds = localBounds_;
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.pass = pass;
  item.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  item.uniforms.push_back(UniformValue::makeMat4("model", model));
  item.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  item.uniforms.push_back(
      UniformValue::makeMat4("projection", camera_.getPerspectiveTransform()));
  item.uniforms.push_back(
      UniformValue::makeVec3("viewPos", camera_.getPosition()));
  const bool useSceneLight =
      ctx.lighting != nullptr && ctx.lighting->directionalEnabled;
  const glm::vec3 sceneLightDirection =
      useSceneLight ? ctx.lighting->direction : lightDirection;
  const glm::vec3 sceneLightColor =
      useSceneLight ? ctx.lighting->color * ctx.lighting->intensity
                    : lightColor;
  const float ambientScale =
      useSceneLight ? ctx.lighting->ambientStrength / 0.42f : 1.0f;
  item.uniforms.push_back(
      UniformValue::makeVec3("lightDirection", glm::normalize(sceneLightDirection)));
  item.uniforms.push_back(
      UniformValue::makeVec3("lightColor", sceneLightColor));
  item.uniforms.push_back(
      UniformValue::makeVec3("materialDiffuse", material.diffuse));
  item.uniforms.push_back(
      UniformValue::makeVec3("materialAmbient", material.ambient * ambientScale));
  item.uniforms.push_back(
      UniformValue::makeVec3("materialSpecular", material.specular));
  item.uniforms.push_back(
      UniformValue::makeFloat("materialShininess", material.shininess));
  submitRenderItem(ctx, *renderDevice_, std::move(item));
}

} // namespace DL
