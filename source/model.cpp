#include "model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace DL {

Model::Model(const std::string &path,
             basist::etc1_global_selector_codebook *codeBook)
    : mCodeBook(codeBook) {
  loadModel(path);
}

void Model::Draw(DL::Shader shader) {
  for (auto &mesh : meshes)
    mesh.Draw(shader);
}

bool Model::hasExtension(std::string_view full, std::string_view end) {
  return full.length() >= end.length() &&
         (full.compare(full.length() - end.length(), end.length(), end) == 0);
}

unsigned int Model::TextureFromFile(const char *path,
                                    const std::string &directory) {
  unsigned char *data = nullptr;
  unsigned int textureID;
  glGenTextures(1, &textureID);
  GLenum format = GL_RGB;
  GLint internal_format = GL_UNSIGNED_SHORT_5_6_5;

  int width, height, nrComponents;

  bool useBasis = false;
  basist::transcoder_texture_format transcoder_format =
      basist::transcoder_texture_format::cTFRGB565;
  std::vector<unsigned char> dst_data;

  if (hasExtension(path, ".basis") && mCodeBook) {
    useBasis = true;
    std::string filename = directory + '/' + path;

    std::ifstream imageStream(filename, std::ios::binary);
    std::vector<unsigned char> buffer(
        (std::istreambuf_iterator<char>(imageStream)), {});

    uint32_t image_index = 0;
    uint32_t level_index = 0;

    auto transcoder = std::make_unique<basist::basisu_transcoder>(mCodeBook);
    transcoder->start_transcoding(buffer.data(), buffer.size());

    basist::basisu_image_info imageInfo{};
    transcoder->get_image_info(buffer.data(), buffer.size(), imageInfo,
                               image_index);

    uint32_t dest_size = 0;
    if (basist::basis_transcoder_format_is_uncompressed(transcoder_format)) {
      const uint32_t bpp =
          basist::basis_get_uncompressed_bytes_per_pixel(transcoder_format);
      dest_size = imageInfo.m_orig_width * imageInfo.m_orig_height * bpp;
    } else {
      uint32_t bpb = basist::basis_get_bytes_per_block(transcoder_format);
      dest_size = imageInfo.m_total_blocks * bpb;
    }

    dst_data.resize(dest_size);

    transcoder->transcode_image_level(
        buffer.data(), buffer.size(), image_index, level_index, dst_data.data(),
        imageInfo.m_orig_width * imageInfo.m_orig_height, transcoder_format);

    data = dst_data.data();
    width = imageInfo.m_width;
    height = imageInfo.m_height;
  } else {
    std::string filename = directory + '/' + path;
    data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    internal_format = GL_UNSIGNED_BYTE;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else
      format = GL_RGBA;
  }

  if (data) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 internal_format, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Texture loaded at path: " << path << std::endl;
    if (!useBasis)
      stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    if (!useBasis)
      stbi_image_free(data);
  }

  return textureID;
}

void Model::loadModel(std::string_view path) {
  tinyobj::ObjReader reader;
  tinyobj::ObjReaderConfig config;
  directory = std::string(path.substr(0, path.find_last_of('/')));
  config.mtl_search_path = directory;

  if (!reader.ParseFromFile(std::string(path), config)) {
    std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
    return;
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader Warning: " << reader.Warning() << std::endl;
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();
  auto &materials = reader.GetMaterials();

  for (const auto &shape : shapes) {
    meshes.push_back(processShape(attrib, shape, materials));
  }
}

Mesh Model::processShape(const tinyobj::attrib_t &attrib,
                         const tinyobj::shape_t &shape,
                         const std::vector<tinyobj::material_t> &materials) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<MeshTexture> textures;

  for (size_t i = 0; i + 2 < shape.mesh.indices.size(); i += 3) {
    Vertex v[3];
    glm::vec3 positions[3];

    for (int j = 0; j < 3; ++j) {
      const auto &idx = shape.mesh.indices[i + j];

      // Position
      positions[j] = glm::vec3(attrib.vertices[3 * idx.vertex_index + 0],
                               attrib.vertices[3 * idx.vertex_index + 1],
                               attrib.vertices[3 * idx.vertex_index + 2]);
      v[j].Position = positions[j];

      // Texture coordinate
      if (idx.texcoord_index >= 0) {
        float tx = attrib.texcoords[2 * idx.texcoord_index + 0];
        float ty = attrib.texcoords[2 * idx.texcoord_index + 1];
        v[j].TexCoords = glm::vec2(tx, 1.0f - ty); // Flip Y
      } else {
        v[j].TexCoords = glm::vec2(0.0f);
      }

      // Placeholder tangent/bitangent
      v[j].Tangent = glm::vec3(0.0f);
      v[j].Bitangent = glm::vec3(0.0f);
    }

    // Calculate flat normal for the triangle
    glm::vec3 normal = glm::normalize(
        glm::cross(positions[1] - positions[0], positions[2] - positions[0]));
    for (int j = 0; j < 3; ++j) {
      v[j].Normal = normal;
      vertices.push_back(v[j]);
      indices.push_back(static_cast<unsigned int>(vertices.size() - 1));
    }
  }

  // Load material (basic fallback)
  Material mat{};
  mat.Diffuse = glm::vec3(0.8f);
  mat.Ambient = glm::vec3(0.4f);
  mat.Specular = glm::vec3(0.2f);
  mat.Shininess = 8.0f;
  mat.Id = 0;

  if (!shape.mesh.material_ids.empty()) {
    int matId = shape.mesh.material_ids[0];
    if (matId >= 0 && matId < materials.size()) {
      const auto &srcMat = materials[matId];
      mat.Diffuse =
          glm::vec3(srcMat.diffuse[0], srcMat.diffuse[1], srcMat.diffuse[2]);
      mat.Ambient =
          glm::vec3(srcMat.ambient[0], srcMat.ambient[1], srcMat.ambient[2]);
      mat.Specular =
          glm::vec3(srcMat.specular[0], srcMat.specular[1], srcMat.specular[2]);
      mat.Shininess = srcMat.shininess;

      if (!srcMat.diffuse_texname.empty()) {
        MeshTexture tex;
        tex.id = TextureFromFile(srcMat.diffuse_texname.c_str(), directory);
        tex.type = "texture_diffuse";
        tex.path = srcMat.diffuse_texname;
        textures.push_back(tex);
        textures_loaded.push_back(tex);
      }
    }
  }

  Mesh mesh(vertices, indices, textures);
  mesh.material = mat;
  return mesh;
}

} // namespace DL