#pragma once

#include <glad/glad.h>

#include "../transcoder/basisu_transcoder.h"
#include "mesh.h"
#include "shader.h"
#include "stb_image.h"
#include "tiny_obj_loader.h" // <-- tinyobjloader for OBJ parsing

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace DL {

class Model {
public:
  std::vector<MeshTexture> textures_loaded;
  std::vector<Mesh> meshes;
  std::string directory;
  basist::etc1_global_selector_codebook *mCodeBook;

  Model(const std::string &path,
        basist::etc1_global_selector_codebook *codeBook);

  void Draw(DL::Shader shader);

private:
  static bool hasExtension(std::string_view full, std::string_view end);

  unsigned int TextureFromFile(const char *path, const std::string &directory);

  // Loads the model and fills the `meshes` vector.
  void loadModel(std::string_view path);

  Mesh processShape(const tinyobj::attrib_t &attrib,
                    const tinyobj::shape_t &shape,
                    const std::vector<tinyobj::material_t> &materials);
};

} // namespace DL