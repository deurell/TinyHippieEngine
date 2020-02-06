#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
public:
  Camera() = default;

  Camera(const glm::vec3 position) : mPosition(position) {}
  Camera(const glm::vec3 position, const glm::quat orientation)
      : mPosition(position), mOrientation(orientation) {}

  glm::mat4 getViewMatrix() {
    return glm::translate(glm::mat4_cast(mOrientation), -mPosition);
  }

  void lookAt(const glm::vec3 position) {
    glm::mat4 lookatMatrix =
        glm::lookAt(mPosition, position, glm::vec3(0.0f, 1.0f, 0.0f));
    mOrientation = lookatMatrix;
  }

  void rotate(float angle, const glm::vec3 &axis) {
    mOrientation *= glm::angleAxis(angle, axis * mOrientation);
  }

  glm::mat4 getPerspectiveTransform(float fov, float aspect) {
    return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
  }

  glm::mat4 getOrtoTransform(float left, float right, float bottom, float up) {
    return glm::ortho(left, right, bottom, up, 0.0f, 100.0f);
  }

  glm::vec3 mPosition;
  glm::quat mOrientation;
};