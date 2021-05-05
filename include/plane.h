//
// Created by Mikael Deurell on 2021-05-05.
//

#pragma once
#include "shader.h"
#include <camera.h>
#include <glad/glad.h>
#include <string_view>

namespace DL {

class Plane {
public:
  Plane(std::string_view vertexShader, std::string_view fragShader, std::string_view glslVersion, DL::Camera& camera);
  ~Plane() = default;

  void render(float delta) const;

  glm::vec3 mPosition {0,0,0};
  GLuint mVAO = 0;
  GLuint mVBO = 0;
  GLuint mEBO = 0;

private:
  std::unique_ptr<DL::Shader> mShader;
  DL::Camera& mCamera;
};

} // namespace DL
