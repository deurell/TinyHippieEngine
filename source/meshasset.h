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
  glm::vec3 diffuseColor{1.0f, 1.0f, 1.0f};
  glm::vec3 ambientColor{0.4f, 0.4f, 0.4f};
  glm::vec3 specularColor{0.2f, 0.2f, 0.2f};
  float shininess = 16.0f;
};

struct MeshAsset {
  std::vector<MeshAssetSubmesh> submeshes;
};

MeshAsset loadMeshAsset(std::string_view path);
MeshAsset loadObjMeshAsset(std::string_view path);
MeshAsset loadGltfMeshAsset(std::string_view path);

} // namespace DL
