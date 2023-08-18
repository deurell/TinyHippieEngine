#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace DL {
class Camera {
public:
  Camera() = default;

  explicit Camera(const glm::vec3 position)
      : mPosition(position), mOrientation(glm::mat4(1)), mFov(45) {}

  Camera(const glm::vec3 position, const glm::quat orientation, const float fov)
      : mPosition(position), mOrientation(orientation), mFov(fov) {}

  [[nodiscard]] glm::mat4 getViewMatrix() const {
    return glm::translate(glm::mat4_cast(mOrientation), -mPosition);
  }

  void lookAt(const glm::vec3 position,
              const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f)) {

    glm::mat4 lookatMatrix = glm::lookAt(mPosition, position, up);
    mOrientation = lookatMatrix;
  }

  void translate(const glm::vec3 &v) { mPosition += v * mOrientation; }
  void translate(float x, float y, float z) {
    mPosition += glm::vec3(x, y, z) * mOrientation;
  }

  void rotate(float angle, const glm::vec3 &axis) {
    mOrientation *= glm::angleAxis(angle, axis * mOrientation);
  }
  void rotate(float angle, float x, float y, float z) {
    mOrientation *= glm::angleAxis(angle, glm::vec3(x, y, z) * mOrientation);
  }

  void yaw(float angle) { rotate(angle, 0.0f, 1.0f, 0.0f); }
  void pitch(float angle) { rotate(angle, 1.0f, 0.0f, 0.0f); }
  void roll(float angle) { rotate(angle, 0.0f, 0.0f, 1.0f); }

  glm::mat4 getPerspectiveTransform() const {
    return glm::perspective(glm::radians(mFov), mScreenSize.x / mScreenSize.y,
                            0.1f, 100.0f);
  }

  static glm::mat4 getOrtoTransform(float left, float right, float bottom,
                                    float up) {
    return glm::ortho(left, right, bottom, up, 0.0f, 100.0f);
  }

  glm::vec3 mPosition = {0, 0, 0};
  glm::quat mOrientation = glm::mat4(1);
  glm::vec2 mScreenSize = {0, 0};
  float mFov = 45;
};
} // namespace DL
