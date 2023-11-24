#include "scenenode.h"
#include <__algorithm/ranges_find_if.h>
#include <glm/gtc/quaternion.hpp>
#include <string_view>
#include <ranges>

namespace DL {

SceneNode::SceneNode(SceneNode *parentNode) : dirty(true), parent(parentNode) {}

void SceneNode::updateTransforms(const glm::mat4 &parentWorldTransform) {
  if (dirty) {
    glm::mat4 positionMatrix = glm::translate(glm::mat4(1.0f), localPosition);
    glm::mat4 rotationMatrix = glm::mat4_cast(localRotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), localScale);

    localTransform = positionMatrix * rotationMatrix * scaleMatrix;
    worldTransform = parentWorldTransform * localTransform;
    dirty = false;
  }

  for (auto &child : children) {
    child->updateTransforms(worldTransform);
  }
}

void SceneNode::setLocalPosition(const glm::vec3 &position) {
  localPosition = position;
  markDirty();
}

glm::vec3 SceneNode::getLocalPosition() const { return localPosition; }

void SceneNode::setLocalRotation(const glm::quat &rotation) {
  localRotation = rotation;
  markDirty();
}

glm::quat SceneNode::getLocalRotation() const { return localRotation; }

void SceneNode::setLocalScale(const glm::vec3 &scale) {
  localScale = scale;
  markDirty();
}

glm::vec3 SceneNode::getLocalScale() const { return localScale; }

void SceneNode::update(float delta) {
  if (parent) {
    updateTransforms(parent->getWorldTransform());
  } else {
    updateTransforms(glm::mat4(1.0f));
  }

  for (auto &child : children) {
    child->update(delta);
  }
}

void SceneNode::render(float delta) {

  for (auto &visualizer : visualizers) {
    visualizer->render(worldTransform, delta);
  }

  for (auto &child : children) {
    child->render(delta);
  }
}

glm::mat4 SceneNode::getWorldTransform() { return worldTransform; }

glm::vec3 SceneNode::getWorldPosition() {
  return glm::vec3(getWorldTransform()[3]);
}

glm::quat SceneNode::getWorldRotation() {
  return glm::quat_cast(getWorldTransform());
}

glm::vec3 SceneNode::getWorldScale() {
  glm::mat4 transform = getWorldTransform();
  return glm::vec3(glm::length(transform[0]), glm::length(transform[1]),
                   glm::length(transform[2]));
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

void SceneNode::addChild(std::unique_ptr<SceneNode> child) {
  child->parent = this;
  children.push_back(std::move(child));
}

void SceneNode::setParent(SceneNode *parentNode) { parent = parentNode; }

void SceneNode::markDirty() {
  dirty = true;
  for (auto &child : children) {
    child->markDirty();
  }
}

VisualizerBase *SceneNode::getVisualizer(std::string_view name) {
  auto iterator = std::ranges::find_if(visualizers, [&name](const auto &visualizer) {
    return visualizer->getName() == name;
  });
  return iterator != visualizers.end() ? iterator->get() : nullptr;
}

} // namespace DL
