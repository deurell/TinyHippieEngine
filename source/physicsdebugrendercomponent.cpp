#include "physicsdebugrendercomponent.h"

#include "physicscontext.h"
#include "renderqueue.h"
#include "scenenode.h"
#include <utility>

PhysicsDebugRenderComponent::PhysicsDebugRenderComponent(
    DL::Camera &camera, DL::SceneNode &node, DL::IRenderDevice &renderDevice,
    const DL::PhysicsContext &physicsContext,
    const std::vector<DL::PhysicsDebugLine> &extraLines)
    : RenderComponent(camera, "Shaders/colored_line.vert",
                     "Shaders/colored_line.frag", node),
      renderDevice_(&renderDevice), physicsContext_(&physicsContext),
      extraLines_(&extraLines) {}

PhysicsDebugRenderComponent::~PhysicsDebugRenderComponent() {
  destroyMesh();
  if (renderDevice_ != nullptr && pipeline_.valid()) {
    renderDevice_->destroy(pipeline_);
  }
}

void PhysicsDebugRenderComponent::destroyMesh() {
  if (renderDevice_ != nullptr && mesh_.valid()) {
    renderDevice_->destroy(mesh_);
    mesh_ = {};
  }
}

void PhysicsDebugRenderComponent::rebuildMesh(
    const std::vector<DL::PhysicsDebugLine> &lines) {
  destroyMesh();
  if (renderDevice_ == nullptr || lines.empty()) {
    return;
  }

  std::vector<glm::vec3> positions;
  std::vector<glm::vec4> colors;
  std::vector<std::uint32_t> indices;
  positions.reserve(lines.size() * 2);
  colors.reserve(lines.size() * 2);
  indices.reserve(lines.size() * 2);

  for (const auto &line : lines) {
    const auto startIndex = static_cast<std::uint32_t>(positions.size());
    positions.push_back(line.start);
    positions.push_back(line.end);
    colors.push_back(line.startColor);
    colors.push_back(line.endColor);
    indices.push_back(startIndex);
    indices.push_back(startIndex + 1);
  }

  mesh_ = renderDevice_->createColoredMesh(positions, colors, indices,
                                           DL::PrimitiveType::Lines);
}

void PhysicsDebugRenderComponent::render(const glm::mat4 &, const DL::FrameContext &ctx,
                                    DL::RenderPassId pass) {
  if (renderDevice_ == nullptr || physicsContext_ == nullptr) {
    return;
  }

  if (!pipeline_.valid()) {
    pipeline_ = renderDevice_->createPipeline(vertexShaderPath_,
                                              fragmentShaderPath_);
  }
  if (!pipeline_.valid()) {
    return;
  }

  if (pass != DL::RenderPassId::Opaque) {
    return;
  }

  auto lines = physicsContext_->getDebugLines();
  const bool hasExtraLines = extraLines_ != nullptr && !extraLines_->empty();
  if (hasExtraLines) {
    lines.insert(lines.end(), extraLines_->begin(), extraLines_->end());
  }

  rebuildMesh(lines);
  if (!mesh_.valid()) {
    return;
  }

  DL::RenderItem item;
  item.tag = DL::RenderTag::Opaque;
  item.renderLayer = node_.renderLayer();
  item.cullable = false;
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.pass = pass;
  item.depthTest = !hasExtraLines;
  item.lineWidth = hasExtraLines ? 4.0f : 2.0f;
  item.uniforms.push_back(
      DL::UniformValue::makeMat4("model", glm::mat4(1.0f)));
  item.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  item.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  DL::submitRenderItem(ctx, *renderDevice_, std::move(item));
}
