#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
public:
  Camera() = default;

  Camera(const glm::vec3 position) : mPosition(position) {}
  Camera(const glm::vec3 position, const glm::quat orientation)
      : mPosition(position), mOrientation(orientation) {}

  glm::mat4 getView() {
    return glm::translate(glm::mat4_cast(mOrientation), mPosition);
  }

  void lookAt(const glm::vec3 position) {
    glm::mat4 lookatMatrix =
        glm::lookAt(-mPosition, position, glm::vec3(0.0f, 1.0f, 0.0f));
    mOrientation = lookatMatrix;
  }

  void rotate(float angle, const glm::vec3 &axis) {
    mOrientation *= glm::angleAxis(angle, axis * mOrientation);
  }

  glm::vec3 mPosition;
  glm::quat mOrientation;
};