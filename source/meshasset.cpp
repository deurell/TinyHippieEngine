#include "meshasset.h"

#include "logger.h"
#include <glm/geometric.hpp>
#include <cmath>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <unordered_map>

namespace DL {
namespace {

struct SubmeshBuilder {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<std::uint32_t> indices;
};

} // namespace

MeshAsset loadObjMeshAsset(std::string_view path) {
  MeshAsset asset;

  tinyobj::ObjReader reader;
  tinyobj::ObjReaderConfig config;
  const std::string objPath(path);
  const auto slash = objPath.find_last_of('/');
  const std::string directory =
      slash == std::string::npos ? std::string(".") : objPath.substr(0, slash);
  config.mtl_search_path = directory;

  if (!reader.ParseFromFile(objPath, config)) {
    LogError("TinyObjReader Error", reader.Error());
    return asset;
  }

  if (!reader.Warning().empty()) {
    LogWarn("TinyObjReader Warning", reader.Warning());
  }

  const auto &attrib = reader.GetAttrib();
  const auto &shapes = reader.GetShapes();
  const auto &materials = reader.GetMaterials();

  for (const auto &shape : shapes) {
    std::unordered_map<int, SubmeshBuilder> builders;
    std::size_t faceVertexOffset = 0;

    for (std::size_t face = 0; face < shape.mesh.num_face_vertices.size();
         ++face) {
      const int faceVertexCount = shape.mesh.num_face_vertices[face];
      if (faceVertexCount != 3) {
        faceVertexOffset += static_cast<std::size_t>(faceVertexCount);
        continue;
      }

      const int materialId =
          face < shape.mesh.material_ids.size() ? shape.mesh.material_ids[face] : -1;
      auto &builder = builders[materialId];
      glm::vec3 facePositions[3];

      for (int vertexInFace = 0; vertexInFace < 3; ++vertexInFace) {
        const auto &index =
            shape.mesh.indices[faceVertexOffset + static_cast<std::size_t>(vertexInFace)];
        facePositions[vertexInFace] = glm::vec3(
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2]);
      }

      glm::vec3 faceNormal = glm::normalize(
          glm::cross(facePositions[1] - facePositions[0],
                     facePositions[2] - facePositions[0]));
      if (!std::isfinite(faceNormal.x) || !std::isfinite(faceNormal.y) ||
          !std::isfinite(faceNormal.z)) {
        faceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
      }

      for (int vertexInFace = 0; vertexInFace < 3; ++vertexInFace) {
        const auto &index =
            shape.mesh.indices[faceVertexOffset + static_cast<std::size_t>(vertexInFace)];
        builder.positions.push_back(facePositions[vertexInFace]);
        builder.normals.push_back(faceNormal);

        if (index.texcoord_index >= 0) {
          builder.uvs.emplace_back(
              attrib.texcoords[2 * index.texcoord_index + 0],
              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
        } else {
          builder.uvs.emplace_back(0.0f, 0.0f);
        }

        builder.indices.push_back(
            static_cast<std::uint32_t>(builder.positions.size() - 1));
      }

      faceVertexOffset += 3;
    }

    for (auto &[materialId, builder] : builders) {
      if (builder.positions.empty()) {
        continue;
      }

      MeshAssetSubmesh submesh;
      submesh.positions = std::move(builder.positions);
      submesh.normals = std::move(builder.normals);
      submesh.uvs = std::move(builder.uvs);
      submesh.indices = std::move(builder.indices);

      if (materialId >= 0 &&
          materialId < static_cast<int>(materials.size())) {
        const auto &material = materials[materialId];
        if (!material.diffuse_texname.empty()) {
          submesh.texturePath = directory + '/' + material.diffuse_texname;
        }
        submesh.fallbackColor =
            glm::vec4(material.diffuse[0], material.diffuse[1],
                      material.diffuse[2], 1.0f);
      }

      asset.submeshes.push_back(std::move(submesh));
    }
  }

  return asset;
}

} // namespace DL
