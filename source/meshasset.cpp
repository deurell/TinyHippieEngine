#include "meshasset.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "logger.h"
#include <glm/geometric.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
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

bool hasExtension(std::string_view path, std::string_view extension) {
  return path.size() >= extension.size() &&
         path.substr(path.size() - extension.size()) == extension;
}

std::string directoryName(std::string_view path) {
  const auto slash = path.find_last_of('/');
  if (slash == std::string_view::npos) {
    return ".";
  }
  return std::string(path.substr(0, slash));
}

std::string resolveUri(std::string_view directory, const char *uri) {
  if (uri == nullptr || uri[0] == '\0') {
    return {};
  }

  const std::string_view uriView(uri);
  if (uriView.find("://") != std::string_view::npos ||
      uriView.rfind("data:", 0) == 0) {
    return {};
  }

  if (directory.empty() || directory == ".") {
    return std::string(uriView);
  }
  return std::string(directory) + '/' + std::string(uriView);
}

AnimationInterpolation toAnimationInterpolation(
    cgltf_interpolation_type interpolation) {
  switch (interpolation) {
  case cgltf_interpolation_type_step:
    return AnimationInterpolation::Step;
  case cgltf_interpolation_type_cubic_spline:
    return AnimationInterpolation::CubicSpline;
  case cgltf_interpolation_type_linear:
  default:
    return AnimationInterpolation::Linear;
  }
}

AnimationTargetPath toAnimationTargetPath(cgltf_animation_path_type path) {
  switch (path) {
  case cgltf_animation_path_type_rotation:
    return AnimationTargetPath::Rotation;
  case cgltf_animation_path_type_scale:
    return AnimationTargetPath::Scale;
  case cgltf_animation_path_type_weights:
    return AnimationTargetPath::Weights;
  case cgltf_animation_path_type_translation:
  default:
    return AnimationTargetPath::Translation;
  }
}

int nodeIndex(const cgltf_data &data, const cgltf_node *node) {
  if (node == nullptr) {
    return -1;
  }
  return static_cast<int>(node - data.nodes);
}

int meshIndex(const cgltf_data &data, const cgltf_mesh *mesh) {
  if (mesh == nullptr) {
    return -1;
  }
  return static_cast<int>(mesh - data.meshes);
}

int skinIndex(const cgltf_data &data, const cgltf_skin *skin) {
  if (skin == nullptr) {
    return -1;
  }
  return static_cast<int>(skin - data.skins);
}

glm::mat4 toMat4(const cgltf_float *matrixData) {
  return glm::make_mat4(matrixData);
}

glm::vec3 transformPosition(const glm::mat4 &transform,
                            const glm::vec3 &position) {
  return glm::vec3(transform * glm::vec4(position, 1.0f));
}

glm::vec3 transformNormal(const glm::mat4 &transform, const glm::vec3 &normal) {
  const glm::mat3 normalMatrix =
      glm::transpose(glm::inverse(glm::mat3(transform)));
  const glm::vec3 transformed = normalMatrix * normal;
  const float length = glm::length(transformed);
  return length > 0.0f ? transformed / length : glm::vec3(0.0f, 1.0f, 0.0f);
}

void generateNormalsIfMissing(MeshAssetSubmesh &submesh) {
  if (!submesh.normals.empty() || submesh.positions.empty()) {
    return;
  }

  submesh.normals.assign(submesh.positions.size(), glm::vec3(0.0f));
  for (std::size_t i = 0; i + 2 < submesh.indices.size(); i += 3) {
    const std::uint32_t i0 = submesh.indices[i + 0];
    const std::uint32_t i1 = submesh.indices[i + 1];
    const std::uint32_t i2 = submesh.indices[i + 2];
    if (i0 >= submesh.positions.size() || i1 >= submesh.positions.size() ||
        i2 >= submesh.positions.size()) {
      continue;
    }

    const glm::vec3 edge1 = submesh.positions[i1] - submesh.positions[i0];
    const glm::vec3 edge2 = submesh.positions[i2] - submesh.positions[i0];
    const glm::vec3 faceNormal = glm::cross(edge1, edge2);
    if (!std::isfinite(faceNormal.x) || !std::isfinite(faceNormal.y) ||
        !std::isfinite(faceNormal.z)) {
      continue;
    }

    submesh.normals[i0] += faceNormal;
    submesh.normals[i1] += faceNormal;
    submesh.normals[i2] += faceNormal;
  }

  for (auto &normal : submesh.normals) {
    const float length = glm::length(normal);
    normal = length > 0.0f ? normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
  }
}

const cgltf_accessor *findAttribute(const cgltf_primitive &primitive,
                                    cgltf_attribute_type type) {
  return cgltf_find_accessor(&primitive, type, 0);
}

void populatePrimitive(const cgltf_data &data, const cgltf_node &node,
                       const cgltf_primitive &primitive,
                       const glm::mat4 &worldTransform,
                       std::string_view assetDirectory, MeshAsset &asset) {
  if (primitive.type != cgltf_primitive_type_triangles) {
    return;
  }

  const cgltf_accessor *positionAccessor =
      findAttribute(primitive, cgltf_attribute_type_position);
  if (positionAccessor == nullptr || positionAccessor->count == 0) {
    return;
  }

  MeshAssetSubmesh submesh;
  submesh.sourceNodeIndex = nodeIndex(data, &node);
  submesh.skinIndex = skinIndex(data, node.skin);
  const glm::mat4 geometryTransform =
      submesh.skinIndex >= 0 ? glm::mat4(1.0f) : worldTransform;
  submesh.positions.resize(positionAccessor->count);
  submesh.uvs.assign(positionAccessor->count, glm::vec2(0.0f));

  const cgltf_accessor *normalAccessor =
      findAttribute(primitive, cgltf_attribute_type_normal);
  if (normalAccessor != nullptr && normalAccessor->count == positionAccessor->count) {
    submesh.normals.resize(positionAccessor->count);
  }

  const cgltf_accessor *uvAccessor =
      findAttribute(primitive, cgltf_attribute_type_texcoord);
  const cgltf_accessor *jointAccessor =
      findAttribute(primitive, cgltf_attribute_type_joints);
  const cgltf_accessor *weightAccessor =
      findAttribute(primitive, cgltf_attribute_type_weights);

  if (jointAccessor != nullptr && weightAccessor != nullptr &&
      jointAccessor->count == positionAccessor->count &&
      weightAccessor->count == positionAccessor->count) {
    submesh.jointIndices.resize(positionAccessor->count);
    submesh.jointWeights.resize(positionAccessor->count, glm::vec4(0.0f));
  }

  for (cgltf_size vertexIndex = 0; vertexIndex < positionAccessor->count;
       ++vertexIndex) {
    cgltf_float positionData[3] = {0.0f, 0.0f, 0.0f};
    if (!cgltf_accessor_read_float(positionAccessor, vertexIndex, positionData, 3)) {
      continue;
    }
    submesh.positions[vertexIndex] =
        transformPosition(geometryTransform,
                          {positionData[0], positionData[1], positionData[2]});

    if (!submesh.normals.empty()) {
      cgltf_float normalData[3] = {0.0f, 0.0f, 1.0f};
      if (cgltf_accessor_read_float(normalAccessor, vertexIndex, normalData, 3)) {
        submesh.normals[vertexIndex] =
            transformNormal(geometryTransform,
                            {normalData[0], normalData[1], normalData[2]});
      }
    }

    if (uvAccessor != nullptr && uvAccessor->count == positionAccessor->count) {
      cgltf_float uvData[2] = {0.0f, 0.0f};
      if (cgltf_accessor_read_float(uvAccessor, vertexIndex, uvData, 2)) {
        submesh.uvs[vertexIndex] = {uvData[0], uvData[1]};
      }
    }

    if (!submesh.jointIndices.empty()) {
      cgltf_uint joints[4] = {0, 0, 0, 0};
      cgltf_float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      if (cgltf_accessor_read_uint(jointAccessor, vertexIndex, joints, 4) &&
          cgltf_accessor_read_float(weightAccessor, vertexIndex, weights, 4)) {
        submesh.jointIndices[vertexIndex] = {
            static_cast<std::uint16_t>(joints[0]),
            static_cast<std::uint16_t>(joints[1]),
            static_cast<std::uint16_t>(joints[2]),
            static_cast<std::uint16_t>(joints[3])};
        submesh.jointWeights[vertexIndex] = {weights[0], weights[1], weights[2],
                                             weights[3]};
      }
    }
  }

  if (primitive.indices != nullptr) {
    submesh.indices.reserve(primitive.indices->count);
    for (cgltf_size index = 0; index < primitive.indices->count; ++index) {
      submesh.indices.push_back(static_cast<std::uint32_t>(
          cgltf_accessor_read_index(primitive.indices, index)));
    }
  } else {
    submesh.indices.reserve(positionAccessor->count);
    for (cgltf_size index = 0; index < positionAccessor->count; ++index) {
      submesh.indices.push_back(static_cast<std::uint32_t>(index));
    }
  }

  if (primitive.material != nullptr) {
    const auto &material = *primitive.material;
    if (material.has_pbr_metallic_roughness) {
      submesh.diffuseColor = {
          material.pbr_metallic_roughness.base_color_factor[0],
          material.pbr_metallic_roughness.base_color_factor[1],
          material.pbr_metallic_roughness.base_color_factor[2]};
    }

    submesh.ambientColor = submesh.diffuseColor * 0.4f;
    submesh.specularColor = glm::vec3(0.15f);
    submesh.shininess = 24.0f;

    const cgltf_texture_view &baseColorTexture =
        material.pbr_metallic_roughness.base_color_texture;
    if (baseColorTexture.texture != nullptr) {
      const cgltf_image *image = nullptr;
      if (baseColorTexture.texture->has_basisu &&
          baseColorTexture.texture->basisu_image != nullptr) {
        image = baseColorTexture.texture->basisu_image;
      } else if (baseColorTexture.texture->image != nullptr) {
        image = baseColorTexture.texture->image;
      }

      if (image != nullptr) {
        submesh.texturePath = resolveUri(assetDirectory, image->uri);
      }
    }
  }

  generateNormalsIfMissing(submesh);
  asset.submeshes.push_back(std::move(submesh));
}

void appendNodeMesh(const cgltf_data &data, const cgltf_node &node,
                    std::string_view assetDirectory, MeshAsset &asset) {
  if (node.mesh == nullptr) {
    return;
  }

  cgltf_float matrixData[16] = {};
  cgltf_node_transform_world(&node, matrixData);
  const glm::mat4 worldTransform = toMat4(matrixData);

  for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count;
       ++primitiveIndex) {
    populatePrimitive(data, node, node.mesh->primitives[primitiveIndex],
                      worldTransform, assetDirectory, asset);
  }
}

void appendSceneNodeMeshes(const cgltf_data &data, const cgltf_node &node,
                           std::string_view assetDirectory, MeshAsset &asset) {
  appendNodeMesh(data, node, assetDirectory, asset);
  for (cgltf_size childIndex = 0; childIndex < node.children_count; ++childIndex) {
    appendSceneNodeMeshes(data, *node.children[childIndex], assetDirectory, asset);
  }
}

void importNodes(const cgltf_data &data, MeshAsset &asset) {
  asset.nodes.reserve(data.nodes_count);
  for (cgltf_size i = 0; i < data.nodes_count; ++i) {
    const auto &srcNode = data.nodes[i];
    cgltf_float localMatrix[16] = {};
    cgltf_float worldMatrix[16] = {};
    cgltf_node_transform_local(&srcNode, localMatrix);
    cgltf_node_transform_world(&srcNode, worldMatrix);

    MeshAssetNode node;
    if (srcNode.name != nullptr) {
      node.name = srcNode.name;
    }
    node.parentIndex = nodeIndex(data, srcNode.parent);
    node.meshIndex = meshIndex(data, srcNode.mesh);
    node.skinIndex = skinIndex(data, srcNode.skin);
    node.localTransform = toMat4(localMatrix);
    node.worldTransform = toMat4(worldMatrix);
    if (srcNode.has_translation) {
      node.baseTranslation = {srcNode.translation[0], srcNode.translation[1],
                              srcNode.translation[2]};
    }
    if (srcNode.has_rotation) {
      node.baseRotation = glm::normalize(glm::quat(
          srcNode.rotation[3], srcNode.rotation[0], srcNode.rotation[1],
          srcNode.rotation[2]));
    }
    if (srcNode.has_scale) {
      node.baseScale = {srcNode.scale[0], srcNode.scale[1], srcNode.scale[2]};
    }
    if (srcNode.has_matrix &&
        (!srcNode.has_translation || !srcNode.has_rotation || !srcNode.has_scale)) {
      glm::vec3 skew;
      glm::vec4 perspective;
      glm::decompose(node.localTransform, node.baseScale, node.baseRotation,
                     node.baseTranslation, skew, perspective);
      node.baseRotation = glm::normalize(node.baseRotation);
    }
    asset.nodes.push_back(std::move(node));
  }
}

void importSkins(const cgltf_data &data, MeshAsset &asset) {
  asset.skins.reserve(data.skins_count);
  for (cgltf_size i = 0; i < data.skins_count; ++i) {
    const auto &srcSkin = data.skins[i];
    MeshAssetSkin skin;
    if (srcSkin.name != nullptr) {
      skin.name = srcSkin.name;
    }
    skin.skeletonRootNodeIndex = nodeIndex(data, srcSkin.skeleton);
    skin.jointNodeIndices.reserve(srcSkin.joints_count);
    for (cgltf_size jointIndex = 0; jointIndex < srcSkin.joints_count; ++jointIndex) {
      skin.jointNodeIndices.push_back(nodeIndex(data, srcSkin.joints[jointIndex]));
    }

    if (srcSkin.inverse_bind_matrices != nullptr) {
      skin.inverseBindMatrices.resize(srcSkin.inverse_bind_matrices->count,
                                      glm::mat4(1.0f));
      for (cgltf_size matrixIndex = 0;
           matrixIndex < srcSkin.inverse_bind_matrices->count; ++matrixIndex) {
        cgltf_float matrixData[16] = {};
        if (cgltf_accessor_read_float(srcSkin.inverse_bind_matrices, matrixIndex,
                                      matrixData, 16)) {
          skin.inverseBindMatrices[matrixIndex] = toMat4(matrixData);
        }
      }
    }

    asset.skins.push_back(std::move(skin));
  }
}

void importAnimations(const cgltf_data &data, MeshAsset &asset) {
  asset.animations.reserve(data.animations_count);
  for (cgltf_size animationIndex = 0; animationIndex < data.animations_count;
       ++animationIndex) {
    const auto &srcAnimation = data.animations[animationIndex];
    AnimationClip clip;
    if (srcAnimation.name != nullptr) {
      clip.name = srcAnimation.name;
    }

    clip.samplers.reserve(srcAnimation.samplers_count);
    for (cgltf_size samplerIndex = 0; samplerIndex < srcAnimation.samplers_count;
         ++samplerIndex) {
      const auto &srcSampler = srcAnimation.samplers[samplerIndex];
      if (srcSampler.input == nullptr || srcSampler.output == nullptr ||
          srcSampler.input->count == 0 || srcSampler.output->count == 0) {
        clip.samplers.emplace_back();
        continue;
      }

      AnimationSampler sampler;
      sampler.interpolation = toAnimationInterpolation(srcSampler.interpolation);
      if (sampler.interpolation == AnimationInterpolation::CubicSpline) {
        LogWarn("Cubic spline animation imported as raw sampler data; evaluator does not yet support tangents");
      }

      sampler.times.resize(srcSampler.input->count);
      for (cgltf_size keyIndex = 0; keyIndex < srcSampler.input->count; ++keyIndex) {
        cgltf_float timeValue = 0.0f;
        if (cgltf_accessor_read_float(srcSampler.input, keyIndex, &timeValue, 1)) {
          sampler.times[keyIndex] = timeValue;
          clip.duration = std::max(clip.duration, timeValue);
        }
      }

      const cgltf_size componentCount = cgltf_num_components(srcSampler.output->type);
      sampler.values.resize(srcSampler.output->count, glm::vec4(0.0f));
      for (cgltf_size keyIndex = 0; keyIndex < srcSampler.output->count; ++keyIndex) {
        cgltf_float value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        if (cgltf_accessor_read_float(srcSampler.output, keyIndex, value,
                                      std::min<cgltf_size>(componentCount, 4))) {
          sampler.values[keyIndex] = {value[0], value[1], value[2], value[3]};
        }
      }

      clip.samplers.push_back(std::move(sampler));
    }

    clip.channels.reserve(srcAnimation.channels_count);
    for (cgltf_size channelIndex = 0; channelIndex < srcAnimation.channels_count;
         ++channelIndex) {
      const auto &srcChannel = srcAnimation.channels[channelIndex];
      if (srcChannel.target_node == nullptr || srcChannel.sampler == nullptr) {
        continue;
      }

      AnimationChannel channel;
      channel.nodeIndex = nodeIndex(data, srcChannel.target_node);
      channel.path = toAnimationTargetPath(srcChannel.target_path);
      channel.samplerIndex =
          static_cast<std::size_t>(srcChannel.sampler - srcAnimation.samplers);
      clip.channels.push_back(std::move(channel));
    }

    asset.animations.push_back(std::move(clip));
  }
}

} // namespace

MeshAsset loadMeshAsset(std::string_view path) {
  if (hasExtension(path, ".obj")) {
    return loadObjMeshAsset(path);
  }
  if (hasExtension(path, ".gltf") || hasExtension(path, ".glb")) {
    return loadGltfMeshAsset(path);
  }

  LogError("Unsupported mesh asset format", path);
  return {};
}

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
        submesh.diffuseColor =
            glm::vec3(material.diffuse[0], material.diffuse[1],
                      material.diffuse[2]);
        submesh.ambientColor =
            glm::vec3(material.ambient[0], material.ambient[1],
                      material.ambient[2]);
        submesh.specularColor =
            glm::vec3(material.specular[0], material.specular[1],
                      material.specular[2]);
        submesh.shininess = material.shininess;
      }

      asset.submeshes.push_back(std::move(submesh));
    }
  }

  return asset;
}

MeshAsset loadGltfMeshAsset(std::string_view path) {
  MeshAsset asset;

  cgltf_options options = {};
  cgltf_data *data = nullptr;
  const std::string gltfPath(path);
  const cgltf_result parseResult =
      cgltf_parse_file(&options, gltfPath.c_str(), &data);
  if (parseResult != cgltf_result_success) {
    LogError("Failed to parse glTF asset", path);
    return asset;
  }

  const auto freeData = [&data]() {
    if (data != nullptr) {
      cgltf_free(data);
      data = nullptr;
    }
  };

  if (cgltf_load_buffers(&options, data, gltfPath.c_str()) !=
      cgltf_result_success) {
    LogError("Failed to load glTF buffers", path);
    freeData();
    return asset;
  }

  if (cgltf_validate(data) != cgltf_result_success) {
    LogWarn("glTF asset validation reported issues", path);
  }

  importNodes(*data, asset);
  importSkins(*data, asset);
  importAnimations(*data, asset);

  const std::string assetDirectory = directoryName(path);
  cgltf_scene *scene = data->scene;
  if (scene == nullptr && data->scenes_count > 0) {
    scene = &data->scenes[0];
  }

  if (scene != nullptr) {
    for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count; ++nodeIndex) {
      appendSceneNodeMeshes(*data, *scene->nodes[nodeIndex], assetDirectory, asset);
    }
  } else {
    for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
      if (data->nodes[nodeIndex].parent == nullptr) {
        appendSceneNodeMeshes(*data, data->nodes[nodeIndex], assetDirectory, asset);
      }
    }
  }

  freeData();

  if (asset.submeshes.empty()) {
    LogWarn("glTF asset did not contain renderable triangle meshes", path);
  }

  return asset;
}

} // namespace DL
