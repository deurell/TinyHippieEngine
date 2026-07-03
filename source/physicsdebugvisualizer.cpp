#include "physicsdebugvisualizer.h"

#include "physicscontext.h"
#include "scenenode.h"

PhysicsDebugVisualizer::PhysicsDebugVisualizer(
    DL::Camera &camera, DL::SceneNode &node, DL::IRenderDevice &renderDevice,
    const DL::PhysicsContext &physicsContext,
    const std::vector<DL::PhysicsDebugLine> &extraLines)
    : VisualizerBase(camera, "Shaders/colored_line.vert",
                     "Shaders/colored_line.frag", node),
      renderDevice_(&renderDevice), physicsContext_(&physicsContext),
      extraLines_(&extraLines) {}

PhysicsDebugVisualizer::~PhysicsDebugVisualizer() {
  destroyMesh();
  if (renderDevice_ != nullptr && pipeline_.valid()) {
    renderDevice_->destroy(pipeline_);
  }
}

void PhysicsDebugVisualizer::destroyMesh() {
  if (renderDevice_ != nullptr && mesh_.valid()) {
    renderDevice_->destroy(mesh_);
    mesh_ = {};
  }
}

void PhysicsDebugVisualizer::rebuildMesh(
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

void PhysicsDebugVisualizer::render(const glm::mat4 &, const DL::FrameContext &,
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

  DL::DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.pass = pass;
  command.depthTest = !hasExtraLines;
  command.lineWidth = hasExtraLines ? 4.0f : 2.0f;
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("model", glm::mat4(1.0f)));
  command.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  renderDevice_->draw(command);
}
