#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace DL {

struct GeneratedMeshData {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;
};

GeneratedMeshData makeCubeMesh();
GeneratedMeshData makeSphereMesh(int latSegments = 16, int lonSegments = 24);
GeneratedMeshData makeCylinderMesh(int radialSegments = 24);

} // namespace DL
