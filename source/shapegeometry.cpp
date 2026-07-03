#include "shapegeometry.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace DL {
namespace {
constexpr float twoPi() { return glm::pi<float>() * 2.0f; }

void appendVertex(GeneratedMeshData &mesh, const glm::vec3 &position,
                  const glm::vec3 &normal, const glm::vec2 &uv) {
  mesh.positions.push_back(position);
  mesh.normals.push_back(normal);
  mesh.uvs.push_back(uv);
}

void appendQuad(GeneratedMeshData &mesh, const glm::vec3 &a, const glm::vec3 &b,
                const glm::vec3 &c, const glm::vec3 &d,
                const glm::vec3 &normal) {
  const std::uint32_t base = static_cast<std::uint32_t>(mesh.positions.size());
  appendVertex(mesh, a, normal, {0.0f, 0.0f});
  appendVertex(mesh, b, normal, {1.0f, 0.0f});
  appendVertex(mesh, c, normal, {1.0f, 1.0f});
  appendVertex(mesh, d, normal, {0.0f, 1.0f});
  mesh.indices.insert(mesh.indices.end(),
                      {base, base + 1, base + 2, base, base + 2, base + 3});
}
} // namespace

GeneratedMeshData makeCubeMesh() {
  GeneratedMeshData mesh;
  mesh.positions.reserve(24);
  mesh.normals.reserve(24);
  mesh.uvs.reserve(24);
  mesh.indices.reserve(36);

  appendQuad(mesh, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
             {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f});
  appendQuad(mesh, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
             {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f});
  appendQuad(mesh, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f},
             {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f});
  appendQuad(mesh, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f},
             {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f});
  appendQuad(mesh, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f},
             {-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f});
  appendQuad(mesh, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
             {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f});

  return mesh;
}

GeneratedMeshData makeSphereMesh(int latSegments, int lonSegments) {
  GeneratedMeshData mesh;
  latSegments = glm::max(latSegments, 3);
  lonSegments = glm::max(lonSegments, 3);

  mesh.positions.reserve(
      static_cast<std::size_t>((latSegments + 1) * (lonSegments + 1)));
  mesh.normals.reserve(mesh.positions.capacity());
  mesh.uvs.reserve(mesh.positions.capacity());

  for (int lat = 0; lat <= latSegments; ++lat) {
    const float v = static_cast<float>(lat) / static_cast<float>(latSegments);
    const float theta = v * glm::pi<float>();
    const float sinTheta = glm::sin(theta);
    const float cosTheta = glm::cos(theta);

    for (int lon = 0; lon <= lonSegments; ++lon) {
      const float u = static_cast<float>(lon) / static_cast<float>(lonSegments);
      const float phi = u * twoPi();
      glm::vec3 normal{glm::cos(phi) * sinTheta, cosTheta,
                       glm::sin(phi) * sinTheta};
      appendVertex(mesh, normal * 0.5f, glm::normalize(normal), {u, v});
    }
  }

  for (int lat = 0; lat < latSegments; ++lat) {
    for (int lon = 0; lon < lonSegments; ++lon) {
      const std::uint32_t row0 =
          static_cast<std::uint32_t>(lat * (lonSegments + 1));
      const std::uint32_t row1 =
          static_cast<std::uint32_t>((lat + 1) * (lonSegments + 1));
      const std::uint32_t a = row0 + static_cast<std::uint32_t>(lon);
      const std::uint32_t b = a + 1;
      const std::uint32_t c = row1 + static_cast<std::uint32_t>(lon) + 1;
      const std::uint32_t d = row1 + static_cast<std::uint32_t>(lon);
      mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
    }
  }

  return mesh;
}

GeneratedMeshData makeCylinderMesh(int radialSegments) {
  GeneratedMeshData mesh;
  radialSegments = glm::max(radialSegments, 3);

  for (int i = 0; i <= radialSegments; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
    const float angle = u * twoPi();
    const float x = glm::cos(angle);
    const float z = glm::sin(angle);
    const glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

    appendVertex(mesh, {x * 0.5f, -0.5f, z * 0.5f}, normal, {u, 0.0f});
    appendVertex(mesh, {x * 0.5f, 0.5f, z * 0.5f}, normal, {u, 1.0f});
  }

  for (int i = 0; i < radialSegments; ++i) {
    const std::uint32_t base = static_cast<std::uint32_t>(i * 2);
    mesh.indices.insert(mesh.indices.end(),
                        {base, base + 1, base + 3, base, base + 3, base + 2});
  }

  const std::uint32_t topCenter =
      static_cast<std::uint32_t>(mesh.positions.size());
  appendVertex(mesh, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f});

  const std::uint32_t bottomCenter =
      static_cast<std::uint32_t>(mesh.positions.size());
  appendVertex(mesh, {0.0f, -0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f});

  const std::uint32_t ringStart =
      static_cast<std::uint32_t>(mesh.positions.size());

  for (int i = 0; i <= radialSegments; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(radialSegments);
    const float angle = u * twoPi();
    const float x = glm::cos(angle) * 0.5f;
    const float z = glm::sin(angle) * 0.5f;

    appendVertex(mesh, {x, 0.5f, z}, {0.0f, 1.0f, 0.0f}, {x + 0.5f, z + 0.5f});
    appendVertex(mesh, {x, -0.5f, z}, {0.0f, -1.0f, 0.0f},
                 {x + 0.5f, z + 0.5f});
  }

  // Cap indices
  for (int i = 0; i < radialSegments; ++i) {
    const std::uint32_t top0 = ringStart + static_cast<std::uint32_t>(i * 2);
    const std::uint32_t bottom0 =
        ringStart + static_cast<std::uint32_t>(i * 2 + 1);
    const std::uint32_t top1 =
        ringStart + static_cast<std::uint32_t>((i + 1) * 2);
    const std::uint32_t bottom1 =
        ringStart + static_cast<std::uint32_t>((i + 1) * 2 + 1);

    mesh.indices.insert(mesh.indices.end(), {topCenter, top0, top1});
    mesh.indices.insert(mesh.indices.end(), {bottomCenter, bottom1, bottom0});
  }

  return mesh;
}
} // namespace DL
