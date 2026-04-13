#include "physicsdebugrenderer.h"

#include <glm/geometric.hpp>

namespace DL {

namespace {

constexpr std::string_view kVertexShaderPath = "Shaders/physicsdebug.vert";
constexpr std::string_view kFragmentShaderPath = "Shaders/physicsdebug.frag";

} // namespace

PhysicsDebugRenderer::PhysicsDebugRenderer(Camera &camera,
                                           IRenderDevice &renderDevice,
                                           RenderResourceCache *resourceCache)
    : camera_(camera), renderDevice_(renderDevice),
      resourceCache_(resourceCache) {
  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(kVertexShaderPath,
                                                    kFragmentShaderPath)
                  : renderDevice_.createPipeline(kVertexShaderPath,
                                                 kFragmentShaderPath);
}

PhysicsDebugRenderer::~PhysicsDebugRenderer() {
  if (mesh_.valid()) {
    renderDevice_.destroy(mesh_);
  }
  if (pipeline_.valid() && resourceCache_ == nullptr) {
    renderDevice_.destroy(pipeline_);
  }
}

void PhysicsDebugRenderer::render(const std::vector<PhysicsDebugLine> &lines) {
  if (!pipeline_.valid()) {
    return;
  }

  if (mesh_.valid()) {
    renderDevice_.destroy(mesh_);
    mesh_ = {};
  }

  if (lines.empty()) {
    return;
  }

  std::vector<glm::vec3> positions;
  std::vector<glm::vec4> colors;
  std::vector<std::uint32_t> indices;
  positions.reserve(lines.size() * 4);
  colors.reserve(lines.size() * 4);
  indices.reserve(lines.size() * 6);

  const glm::vec3 cameraPosition = camera_.getPosition();
  const float thickness = 0.035f;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const glm::vec3 line = lines[i].end - lines[i].start;
    const float lineLength = glm::length(line);
    if (lineLength <= 0.0001f) {
      continue;
    }

    const glm::vec3 lineDir = line / lineLength;
    const glm::vec3 midPoint = (lines[i].start + lines[i].end) * 0.5f;
    glm::vec3 viewDir = glm::normalize(cameraPosition - midPoint);
    if (glm::dot(viewDir, viewDir) <= 0.000001f) {
      viewDir = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 side = glm::cross(lineDir, viewDir);
    if (glm::dot(side, side) <= 0.000001f) {
      side = glm::cross(lineDir, glm::vec3(0.0f, 1.0f, 0.0f));
      if (glm::dot(side, side) <= 0.000001f) {
        side = glm::cross(lineDir, glm::vec3(1.0f, 0.0f, 0.0f));
      }
    }
    side = glm::normalize(side) * thickness;

    const auto vertexBase = static_cast<std::uint32_t>(positions.size());
    positions.push_back(lines[i].start - side);
    positions.push_back(lines[i].start + side);
    positions.push_back(lines[i].end + side);
    positions.push_back(lines[i].end - side);

    colors.push_back(lines[i].startColor);
    colors.push_back(lines[i].startColor);
    colors.push_back(lines[i].endColor);
    colors.push_back(lines[i].endColor);

    indices.push_back(vertexBase);
    indices.push_back(vertexBase + 1);
    indices.push_back(vertexBase + 2);
    indices.push_back(vertexBase);
    indices.push_back(vertexBase + 2);
    indices.push_back(vertexBase + 3);
  }

  if (positions.empty()) {
    return;
  }

  mesh_ = renderDevice_.createColoredMesh(positions, colors, indices,
                                          PrimitiveType::Triangles);
  if (!mesh_.valid()) {
    return;
  }

  DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.blendMode = BlendMode::Alpha;
  command.uniforms.push_back(
      UniformValue::makeMat4("view", camera_.getViewMatrix()));
  command.uniforms.push_back(
      UniformValue::makeMat4("projection", camera_.getPerspectiveTransform()));
  renderDevice_.draw(command);
}

} // namespace DL
