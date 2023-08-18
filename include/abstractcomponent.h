//
// Created by Mikael Deurell on 2023-08-15.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "camera.h"
#include <string_view>
#include <string>
#include <utility>

namespace DL {
class AbstractComponent {
public:
  AbstractComponent(DL::Camera &camera, std::string name,
                    std::string glslVersionString,
                    std::string vertexShaderPath,
                    std::string fragmentShaderPath)
      : camera_(camera), name_(std::move(name)),
        glslVersionString_(std::move(glslVersionString)),
        vertexShaderPath_(std::move(vertexShaderPath)),
        fragmentShaderPath_(std::move(fragmentShaderPath)) {}

  virtual void init() = 0;
  virtual void render(const glm::mat4 &worldTransform, float delta) = 0;
  virtual ~AbstractComponent() = default;

  static glm::mat4 normalizeRotation(const glm::mat4 &matrix) {
    glm::mat4 normalizedMatrix = matrix;
    normalizedMatrix[0] = glm::normalize(matrix[0]);
    normalizedMatrix[1] = glm::normalize(matrix[1]);
    normalizedMatrix[2] = glm::normalize(matrix[2]);
    return normalizedMatrix;
  }

protected:
  static glm::quat extractRotation(const glm::mat4 &matrix) {
    glm::mat4 normalizedMatrix = normalizeRotation(matrix);
    return glm::quat_cast(normalizedMatrix);
  }

  static glm::vec3 extractScale(const glm::mat4 &matrix) {
    glm::vec3 scale;
    scale.x = glm::length(matrix[0]);
    scale.y = glm::length(matrix[1]);
    scale.z = glm::length(matrix[2]);
    return scale;
  }

  static glm::vec3 extractPosition(const glm::mat4 &matrix) {
    return glm::vec3(matrix[3]);
  }

  DL::Camera &camera_;
  std::string name_;
  std::string glslVersionString_;
  std::string vertexShaderPath_;
  std::string fragmentShaderPath_;
};
} // namespace DL
