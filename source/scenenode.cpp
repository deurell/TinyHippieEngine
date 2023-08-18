#include "scenenode.h"
#include <glm/gtc/quaternion.hpp>

namespace DL {

void SceneNode::updateTransforms(const glm::mat4 &parentWorldTransform) {
  if (dirty) {
    worldTransform = parentWorldTransform * localTransform;
    dirty = false;
  }

  for (auto &child : children) {
    child->updateTransforms(worldTransform);
  }
}

void SceneNode::render(float delta) {
  updateTransforms(glm::mat4(1.0f));

  for (auto &component : components) {
    component->render(worldTransform, delta);
  }

  for (auto &child : children) {
    child->render(delta);
  }
}

void SceneNode::setLocalPosition(const glm::vec3 &position) {
  localTransform[3] = glm::vec4(position, 1.0f);
  dirty = true;
}

glm::vec3 SceneNode::getLocalPosition() const {
  return glm::vec3(localTransform[3]);
}

void SceneNode::setLocalRotation(const glm::quat &rotation) {
  glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
  localTransform = extractPositionMatrix() * rotationMatrix * extractScaleMatrix();
  dirty = true;
}

glm::quat SceneNode::getLocalRotation() const {
  return glm::quat_cast(localTransform);
}

void SceneNode::setLocalScale(const glm::vec3 &scale) {
  glm::mat4 scaleMatrix(1.0f);
  scaleMatrix[0][0] = scale.x;
  scaleMatrix[1][1] = scale.y;
  scaleMatrix[2][2] = scale.z;
  localTransform = extractPositionMatrix() * glm::mat4_cast(getLocalRotation()) * scaleMatrix;
  dirty = true;
}

glm::vec3 SceneNode::getLocalScale() const {
  return glm::vec3(glm::length(localTransform[0]),
                   glm::length(localTransform[1]),
                   glm::length(localTransform[2]));
}

glm::mat4 SceneNode::extractPositionMatrix() const {
  glm::mat4 positionMatrix = glm::mat4(1.0f);
  positionMatrix[3] = localTransform[3];
  return positionMatrix;
}

glm::mat4 SceneNode::extractScaleMatrix() const {
  glm::vec3 scale = getLocalScale();
  glm::mat4 scaleMatrix(1.0f);
  scaleMatrix[0][0] = scale.x;
  scaleMatrix[1][1] = scale.y;
  scaleMatrix[2][2] = scale.z;
  return scaleMatrix;
}

void SceneNode::init() {}

void SceneNode::onClick(double x, double y) {}

void SceneNode::onKey(int key) {}

void SceneNode::onScreenSizeChanged(glm::vec2 size) {
  for (auto &child : children) {
    child->onScreenSizeChanged(size);
  }
}

SceneNode::SceneNode() : dirty(true) {}

} // namespace DL
