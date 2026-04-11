#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace DL {

struct MeshAssetSubmesh {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;
  std::string texturePath;
  glm::vec4 fallbackColor{1.0f, 1.0f, 1.0f, 1.0f};
};

struct MeshAsset {
  std::vector<MeshAssetSubmesh> submeshes;
};

MeshAsset loadObjMeshAsset(std::string_view path);

} // namespace DL
