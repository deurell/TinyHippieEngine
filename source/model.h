#pragma once

#include <glad/glad.h>

#include "../transcoder/basisu_transcoder.h"
#include "mesh.h"
#include "shader.h"
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace DL {
class Model {
public:
  /*  Model Data */
  vector<Texture>
      textures_loaded; // stores all the textures loaded so far, optimization to
                       // make sure textures aren't loaded more than once.
  vector<Mesh> meshes;
  string directory;
  basist::etc1_global_selector_codebook *mCodeBook;

  /*  Functions   */
  // constructor, expects a filepath to a 3D model.
  Model(string const &path, basist::etc1_global_selector_codebook *codeBook) {
    mCodeBook = codeBook;
    loadModel(path);
  }

  // draws the model, and thus all its meshes
  void Draw(DL::Shader shader) {
    for (unsigned int i = 0; i < meshes.size(); i++)
      meshes[i].Draw(shader);
  }

private:
  bool hasExtension(const std::string &full, const std::string &end) {
    if (full.length() >= end.length()) {
      return (full.compare(full.length() - end.length(), full.length(), end) ==
              0);
    } else {
      return false;
    }
  }
  unsigned int TextureFromFile(const char *path, const string &directory) {
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
      string filename = string(path);
      filename = directory + '/' + filename;

      std::ifstream imageStream(filename, std::ios::binary);
      std::vector<unsigned char> buffer(
          std::istreambuf_iterator<char>(imageStream), {});

      uint32_t image_index = 0;
      uint32_t level_index = 0;

      auto transcoder = std::make_unique<basist::basisu_transcoder>(mCodeBook);
      bool success =
          transcoder->start_transcoding(buffer.data(), buffer.size());
      basist::basis_texture_type textureType =
          transcoder->get_texture_type(buffer.data(), buffer.size());
      uint32_t imageCount =
          transcoder->get_total_images(buffer.data(), buffer.size());

      basist::basisu_image_info imageInfo;
      transcoder->get_image_info(buffer.data(), buffer.size(), imageInfo,
                                 image_index);

      basist::basisu_image_level_info levelInfo;
      transcoder->get_image_level_info(buffer.data(), buffer.size(), levelInfo,
                                       image_index, level_index);
      uint32_t dest_size = 0;
      bool unCompressed = false;
      if (basist::basis_transcoder_format_is_uncompressed(transcoder_format)) {
        unCompressed = true;
        const uint32_t bytes_per_pixel =
            basist::basis_get_uncompressed_bytes_per_pixel(transcoder_format);
        const uint32_t bytes_per_line =
            imageInfo.m_orig_width * bytes_per_pixel;
        const uint32_t bytes_per_slice =
            bytes_per_line * imageInfo.m_orig_height;
        dest_size = bytes_per_slice;
      } else {
        uint32_t bytes_per_block =
            basist::basis_get_bytes_per_block(transcoder_format);
        uint32_t required_size = imageInfo.m_total_blocks * bytes_per_block;

        if (transcoder_format ==
                basist::transcoder_texture_format::cTFPVRTC1_4_RGB ||
            transcoder_format ==
                basist::transcoder_texture_format::cTFPVRTC1_4_RGBA) {
          // For PVRTC1, Basis only writes (or requires) total_blocks *
          // bytes_per_block. But GL requires extra padding for very small
          // textures:
          // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
          // The transcoder will clear the extra bytes followed the used blocks
          // to 0.
          const uint32_t width = (imageInfo.m_orig_width + 3) & ~3;
          const uint32_t height = (imageInfo.m_orig_height + 3) & ~3;
          required_size =
              (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
        }
        dest_size = required_size;
      }

      dst_data.resize(dest_size);

      bool status = transcoder->transcode_image_level(
          buffer.data(), buffer.size(), 0, 0, dst_data.data(),
          imageInfo.m_orig_width * imageInfo.m_orig_height, transcoder_format,
          0, imageInfo.m_orig_width, nullptr, imageInfo.m_orig_height);
      data = dst_data.data();
      width = imageInfo.m_width;
      height = imageInfo.m_height;
    } else {
      string filename = string(path);
      filename = directory + '/' + filename;
      data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
      internal_format = GL_UNSIGNED_BYTE;
      if (nrComponents == 1) {
        format = GL_RED;
      } else if (nrComponents == 3) {
        format = GL_RGB;
      } else {
        format = GL_RGBA;
      }
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
      if (!useBasis) {
        stbi_image_free(data);
      }
    } else {
      std::cout << "Texture failed to load at path: " << path << std::endl;
      if (!useBasis) {
        stbi_image_free(data);
      }
    }

    return textureID;
  }

  /*  Functions   */
  // loads a model with supported ASSIMP extensions from file and stores the
  // resulting meshes in the meshes vector.
  void loadModel(string const &path) {
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
                                    aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) // if is Not Zero
    {
      cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
      return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
  }

  // processes a node in a recursive fashion. Processes each individual mesh
  // located at the node and repeats this process on its children nodes (if
  // any).
  void processNode(aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      // the node object only contains indices to index the actual objects in
      // the scene. the scene contains all the data, node is just to keep
      // stuff organized (like relations between nodes).
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively
    // process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene);
    }
  }

  Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // Walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      glm::vec3 vector; // we declare a placeholder vector since assimp uses
                        // its own vector class that doesn't directly convert
                        // to glm's vec3 class so we transfer the data to this
                        // placeholder glm::vec3 first.
      // positions
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.Position = vector;
      // normals
      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
      vertex.Normal = vector;
      // texture coordinates
      if (mesh->mTextureCoords[0]) // does the mesh contain texture
                                   // coordinates?
      {
        glm::vec2 vec;
        // a vertex can contain up to 8 different texture coordinates. We thus
        // make the assumption that we won't use models where a vertex can
        // have multiple texture coordinates so we always take the first set
        // (0).
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.TexCoords = vec;
      } else
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);
      // tangent
      vector.x = mesh->mTangents[i].x;
      vector.y = mesh->mTangents[i].y;
      vector.z = mesh->mTangents[i].z;
      vertex.Tangent = vector;
      // bitangent
      vector.x = mesh->mBitangents[i].x;
      vector.y = mesh->mBitangents[i].y;
      vector.z = mesh->mBitangents[i].z;
      vertex.Bitangent = vector;
      vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its
    // triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      // retrieve all indices of the face and store them in the indices vector
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse
    // texture should be named as 'texture_diffuseN' where N is a sequential
    // number ranging from 1 to MAX_SAMPLER_NUMBER. Same applies to other
    // texture as the following list summarizes: diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(
        material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(
        material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps =
        loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps =
        loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // return a mesh object created from the extracted mesh data

    Mesh meshToReturn = Mesh(vertices, indices, textures);
    aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
    Material mappedMaterial;
    aiColor3D color(0.f, 0.f, 0.f);
    float shininess;

    mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    mappedMaterial.Diffuse = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
    mappedMaterial.Ambient = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
    mappedMaterial.Specular = glm::vec3(color.r, color.b, color.g);

    mat->Get(AI_MATKEY_SHININESS, shininess);
    mappedMaterial.Shininess = shininess;
    mappedMaterial.Id = mesh->mMaterialIndex;
    meshToReturn.material = mappedMaterial;

    return meshToReturn;
  }

  // checks all material textures of a given type and loads the textures if
  // they're not loaded yet. the required info is returned as a Texture
  // struct.
  vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                       string typeName) {
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
      aiString str;
      mat->GetTexture(type, i, &str);
      // check if texture was loaded before and if so, continue to next
      // iteration: skip loading a new texture
      bool skip = false;
      for (unsigned int j = 0; j < textures_loaded.size(); j++) {
        if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
          textures.push_back(textures_loaded[j]);
          skip = true; // a texture with the same filepath has already been
                       // loaded, continue to next one. (optimization)
          break;
        }
      }
      if (!skip) { // if texture hasn't been loaded already, load it
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), this->directory);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
        textures_loaded.push_back(
            texture); // store it as texture loaded for entire model, to
                      // ensure we won't unnecesery load duplicate textures.
      }
    }
    return textures;
  }
};
}; // namespace DL