//
// Created by Mikael Deurell on 2021-05-05.
//

#pragma once
#include "shader.h"
#include <camera.h>
#include <functional>
#include <glad/glad.h>
#include <memory>
#include <string_view>

namespace DL {

class Plane {
public:
  Plane(std::unique_ptr<DL::Shader> shader, DL::Camera &camera,
        const std::function<void(DL::Shader &)> &shaderModifier = nullptr);

  ~Plane() = default;

  void render(float delta) const;

  glm::vec3 position{0, 0, 0};
  glm::vec3 scale{1.0, 1.0, 1.0};
  GLuint mVAO = 0;
  GLuint mVBO = 0;
  GLuint mEBO = 0;

private:
  std::unique_ptr<DL::Shader> mShader;
  DL::Camera &mCamera;
  const std::function<void(DL::Shader &)> mShaderModifier;
};

} // namespace DL
 