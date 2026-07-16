#include "physicsmeshvisualizer.h"

#include "scenenode.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <limits>
#include <utility>

namespace {

struct MeshData {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<glm::vec4> colors;
  std::vector<std::uint32_t> indices;
};

void pushVertex(MeshData &mesh, const glm::vec3 &position,
                const glm::vec3 &normal, const glm::vec2 &uv,
                const glm::vec4 &color) {
  mesh.positions.push_back(position);
  mesh.normals.push_back(normal);
  mesh.uvs.push_back(uv);
  mesh.colors.push_back(color);
}

MeshData makeBoxMesh(const glm::vec3 &halfExtents, const glm::vec4 &color) {
  MeshData mesh;

  struct Face {
    glm::vec3 normal;
    glm::vec3 corners[4];
  };

  const Face faces[] = {
      {{0.0f, 0.0f, -1.0f},
       {{-halfExtents.x, -halfExtents.y, -halfExtents.z},
        {halfExtents.x, -halfExtents.y, -halfExtents.z},
        {halfExtents.x, halfExtents.y, -halfExtents.z},
        {-halfExtents.x, halfExtents.y, -halfExtents.z}}},
      {{0.0f, 0.0f, 1.0f},
       {{halfExtents.x, -halfExtents.y, halfExtents.z},
        {-halfExtents.x, -halfExtents.y, halfExtents.z},
        {-halfExtents.x, halfExtents.y, halfExtents.z},
        {halfExtents.x, halfExtents.y, halfExtents.z}}},
      {{0.0f, -1.0f, 0.0f},
       {{-halfExtents.x, -halfExtents.y, halfExtents.z},
        {halfExtents.x, -halfExtents.y, halfExtents.z},
        {halfExtents.x, -halfExtents.y, -halfExtents.z},
        {-halfExtents.x, -halfExtents.y, -halfExtents.z}}},
      {{0.0f, 1.0f, 0.0f},
       {{-halfExtents.x, halfExtents.y, -halfExtents.z},
        {halfExtents.x, halfExtents.y, -halfExtents.z},
        {halfExtents.x, halfExtents.y, halfExtents.z},
        {-halfExtents.x, halfExtents.y, halfExtents.z}}},
      {{-1.0f, 0.0f, 0.0f},
       {{-halfExtents.x, -halfExtents.y, halfExtents.z},
        {-halfExtents.x, -halfExtents.y, -halfExtents.z},
        {-halfExtents.x, halfExtents.y, -halfExtents.z},
        {-halfExtents.x, halfExtents.y, halfExtents.z}}},
      {{1.0f, 0.0f, 0.0f},
       {{halfExtents.x, -halfExtents.y, -halfExtents.z},
        {halfExtents.x, -halfExtents.y, halfExtents.z},
        {halfExtents.x, halfExtents.y, halfExtents.z},
        {halfExtents.x, halfExtents.y, -halfExtents.z}}},
  };

  const glm::vec2 uvs[] = {{0.0f, 0.0f}, {1.0f, 0.0f},
                           {1.0f, 1.0f}, {0.0f, 1.0f}};
  for (const auto &face : faces) {
    const auto start = static_cast<std::uint32_t>(mesh.positions.size());
    for (int i = 0; i < 4; ++i) {
      pushVertex(mesh, face.corners[i], face.normal, uvs[i], color);
    }
    mesh.indices.insert(mesh.indices.end(),
                        {start, start + 1, start + 2, start + 2, start + 3,
                         start});
  }
  return mesh;
}

MeshData makeSphereMesh(float radius, const glm::vec4 &color) {
  constexpr int kRings = 10;
  constexpr int kSegments = 18;
  MeshData mesh;

  for (int ring = 0; ring <= kRings; ++ring) {
    const float v = static_cast<float>(ring) / static_cast<float>(kRings);
    const float phi = v * glm::pi<float>();
    const float y = std::cos(phi) * radius;
    const float ringRadius = std::sin(phi) * radius;
    for (int segment = 0; segment <= kSegments; ++segment) {
      const float u =
          static_cast<float>(segment) / static_cast<float>(kSegments);
      const float theta = u * glm::two_pi<float>();
      const glm::vec3 position{std::cos(theta) * ringRadius, y,
                               std::sin(theta) * ringRadius};
      pushVertex(mesh, position, glm::normalize(position), {u, v}, color);
    }
  }

  const int stride = kSegments + 1;
  for (int ring = 0; ring < kRings; ++ring) {
    for (int segment = 0; segment < kSegments; ++segment) {
      const auto a = static_cast<std::uint32_t>(ring * stride + segment);
      const auto b = static_cast<std::uint32_t>((ring + 1) * stride + segment);
      const auto c = static_cast<std::uint32_t>(b + 1);
      const auto d = static_cast<std::uint32_t>(a + 1);
      mesh.indices.insert(mesh.indices.end(), {a, b, d, d, b, c});
    }
  }
  return mesh;
}

MeshData makeCapsuleMesh(float radius, float height, const glm::vec4 &color) {
  constexpr int kSegments = 18;
  MeshData mesh;
  const float halfHeight = height * 0.5f;

  for (int yIndex = 0; yIndex <= 1; ++yIndex) {
    const float y = yIndex == 0 ? -halfHeight : halfHeight;
    for (int segment = 0; segment <= kSegments; ++segment) {
      const float theta = (static_cast<float>(segment) /
                           static_cast<float>(kSegments)) *
                          glm::two_pi<float>();
      const glm::vec3 radial{std::cos(theta), 0.0f, std::sin(theta)};
      pushVertex(mesh, {radial.x * radius, y, radial.z * radius}, radial,
                 {static_cast<float>(segment) / static_cast<float>(kSegments),
                  static_cast<float>(yIndex)},
                 color);
    }
  }

  const int stride = kSegments + 1;
  for (int segment = 0; segment < kSegments; ++segment) {
    const auto a = static_cast<std::uint32_t>(segment);
    const auto b = static_cast<std::uint32_t>(stride + segment);
    const auto c = static_cast<std::uint32_t>(b + 1);
    const auto d = static_cast<std::uint32_t>(a + 1);
    mesh.indices.insert(mesh.indices.end(), {a, b, d, d, b, c});
  }

  auto sphere = makeSphereMesh(radius, color);
  const std::uint32_t topOffset =
      static_cast<std::uint32_t>(mesh.positions.size());
  for (auto position : sphere.positions) {
    position.y += halfHeight;
    pushVertex(mesh, position, glm::normalize(position - glm::vec3(0.0f, halfHeight, 0.0f)),
               {0.0f, 0.0f}, color);
  }
  for (auto index : sphere.indices) {
    mesh.indices.push_back(topOffset + index);
  }

  const std::uint32_t bottomOffset =
      static_cast<std::uint32_t>(mesh.positions.size());
  for (auto position : sphere.positions) {
    position.y -= halfHeight;
    pushVertex(mesh, position, glm::normalize(position - glm::vec3(0.0f, -halfHeight, 0.0f)),
               {0.0f, 0.0f}, color);
  }
  for (auto index : sphere.indices) {
    mesh.indices.push_back(bottomOffset + index);
  }
  return mesh;
}

DL::Bounds boundsFromPositions(const std::vector<glm::vec3> &positions) {
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

PhysicsMeshVisualizer::PhysicsMeshVisualizer(
    DL::Camera &camera, DL::SceneNode &node, DL::IRenderDevice &renderDevice,
    const DL::PhysicsShapeDesc &shape, const glm::vec4 &color)
    : VisualizerBase(camera, "Shaders/colored_line.vert",
                     "Shaders/colored_line.frag", node),
      renderDevice_(&renderDevice), baseTint_(color), ambientTint_(glm::vec3(color) * 0.5f + glm::vec3(0.2f)) {
  vertexShaderPath_ = "Shaders/meshnode.vert";
  fragmentShaderPath_ = "Shaders/meshnode.frag";
  createMesh(shape, color);
}

PhysicsMeshVisualizer::~PhysicsMeshVisualizer() {
  if (renderDevice_ != nullptr && mesh_.valid()) {
    renderDevice_->destroy(mesh_);
  }
  if (renderDevice_ != nullptr && texture_.valid()) {
    renderDevice_->destroy(texture_);
  }
  if (renderDevice_ != nullptr && pipeline_.valid()) {
    renderDevice_->destroy(pipeline_);
  }
}

void PhysicsMeshVisualizer::createMesh(const DL::PhysicsShapeDesc &shape,
                                       const glm::vec4 &color) {
  if (renderDevice_ == nullptr) {
    return;
  }

  MeshData mesh;
  switch (shape.type) {
  case DL::PhysicsShapeType::Box:
    mesh = makeBoxMesh(shape.halfExtents, color);
    break;
  case DL::PhysicsShapeType::Sphere:
    mesh = makeSphereMesh(shape.radius, color);
    break;
  case DL::PhysicsShapeType::Capsule:
    mesh = makeCapsuleMesh(shape.radius, shape.height, color);
    break;
  }
  localBounds_ = boundsFromPositions(mesh.positions);
  mesh_ = renderDevice_->createMesh(mesh.positions, mesh.normals, mesh.uvs,
                                    mesh.indices);
  const std::uint8_t whitePixel[] = {255, 255, 255, 255};
  texture_ = renderDevice_->createTexture(
      {.pixels = whitePixel,
       .width = 1,
       .height = 1,
       .format = DL::TextureFormat::RGBA8,
       .generateMipmaps = false});
}

void PhysicsMeshVisualizer::render(const glm::mat4 &worldTransform,
                                   const DL::FrameContext &ctx,
                                   DL::RenderPassId pass) {
  if (pass != DL::RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !texture_.valid()) {
    return;
  }

  if (!pipeline_.valid()) {
    pipeline_ = renderDevice_->createPipeline(vertexShaderPath_,
                                              fragmentShaderPath_);
  }
  if (!pipeline_.valid()) {
    return;
  }

  DL::RenderItem item;
  item.tag = DL::RenderTag::Opaque;
  item.localBounds = localBounds_;
  item.mesh = mesh_;
  item.pipeline = pipeline_;
  item.texture = texture_;
  item.pass = pass;
  item.uniforms.push_back(
      DL::UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  item.uniforms.push_back(DL::UniformValue::makeMat4("model", worldTransform));
  item.uniforms.push_back(
      DL::UniformValue::makeMat4("view", camera_.getViewMatrix()));
  item.uniforms.push_back(DL::UniformValue::makeMat4(
      "projection", camera_.getPerspectiveTransform()));
  item.uniforms.push_back(
      DL::UniformValue::makeVec3("viewPos", camera_.getPosition()));
  item.uniforms.push_back(DL::UniformValue::makeVec3(
      "lightDirection", glm::normalize(glm::vec3(0.35f, 1.0f, 0.42f))));
  item.uniforms.push_back(
      DL::UniformValue::makeVec3("lightColor", {1.0f, 0.96f, 0.9f}));
  item.uniforms.push_back(
      DL::UniformValue::makeFloat("ambientStrength", 0.58f));
  item.uniforms.push_back(
      DL::UniformValue::makeFloat("specularStrength", 0.24f));
  item.uniforms.push_back(DL::UniformValue::makeFloat("shininess", 22.0f));
  item.uniforms.push_back(DL::UniformValue::makeVec3("baseTint", baseTint_));
  item.uniforms.push_back(
      DL::UniformValue::makeVec3("ambientTint", ambientTint_));
  item.uniforms.push_back(DL::UniformValue::makeInt("debugNormals", 0));
  item.uniforms.push_back(DL::UniformValue::makeInt("useSkinning", 0));
  DL::submitRenderItem(ctx, *renderDevice_, std::move(item));
}
