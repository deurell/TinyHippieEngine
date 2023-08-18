//
// Created by Mikael Deurell on 2023-08-15.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace DL {
class AbstractComponent {
public:
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
};
} // namespace DL
