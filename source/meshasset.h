#pragma once

#include "animationclip.h"
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace DL {

struct MeshAssetNode {
  std::string name;
  int parentIndex = -1;
  int meshIndex = -1;
  int skinIndex = -1;
  glm::vec3 baseTranslation{0.0f, 0.0f, 0.0f};
  glm::quat baseRotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 baseScale{1.0f, 1.0f, 1.0f};
  glm::mat4 localTransform{1.0f};
  glm::mat4 worldTransform{1.0f};
};

struct MeshAssetSkin {
  std::string name;
  int skeletonRootNodeIndex = -1;
  std::vector<int> jointNodeIndices;
  std::vector<glm::mat4> inverseBindMatrices;
};

struct MeshAssetSubmesh {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;
  std::vector<std::array<std::uint16_t, 4>> jointIndices;
  std::vector<glm::vec4> jointWeights;
  std::string texturePath;
  glm::vec3 diffuseColor{1.0f, 1.0f, 1.0f};
  glm::vec3 ambientColor{0.4f, 0.4f, 0.4f};
  glm::vec3 specularColor{0.2f, 0.2f, 0.2f};
  float shininess = 16.0f;
  int sourceNodeIndex = -1;
  int skinIndex = -1;
};

struct MeshAsset {
  std::vector<MeshAssetNode> nodes;
  std::vector<MeshAssetSkin> skins;
  std::vector<AnimationClip> animations;
  std::vector<MeshAssetSubmesh> submeshes;
};

MeshAsset loadMeshAsset(std::string_view path);
MeshAsset loadObjMeshAsset(std::string_view path);
MeshAsset loadGltfMeshAsset(std::string_view path);

} // namespace DL
