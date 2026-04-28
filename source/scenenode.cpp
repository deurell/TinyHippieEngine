#include "scenenode.h"
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <ranges>
#include <string_view>

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
}

void SceneNode::applyLocalPosition(const glm::vec3 &position) {
  localPosition = position;
  markDirty();
}

void SceneNode::setLocalPosition(const glm::vec3 &position) {
  if (debugTransformOverrideEnabled_) {
    return;
  }
  applyLocalPosition(position);
}

glm::vec3 SceneNode::getLocalPosition() const { return localPosition; }

void SceneNode::setDebugLocalPosition(const glm::vec3 &position) {
  applyLocalPosition(position);
}

void SceneNode::applyLocalRotation(const glm::quat &rotation) {
  localRotation = rotation;
  markDirty();
}

void SceneNode::setLocalRotation(const glm::quat &rotation) {
  if (debugTransformOverrideEnabled_) {
    return;
  }
  applyLocalRotation(rotation);
}

glm::quat SceneNode::getLocalRotation() const { return localRotation; }

void SceneNode::setDebugLocalRotation(const glm::quat &rotation) {
  applyLocalRotation(rotation);
}

void SceneNode::applyLocalScale(const glm::vec3 &scale) {
  localScale = scale;
  markDirty();
}

void SceneNode::setLocalScale(const glm::vec3 &scale) {
  if (debugTransformOverrideEnabled_) {
    return;
  }
  applyLocalScale(scale);
}

glm::vec3 SceneNode::getLocalScale() const { return localScale; }

void SceneNode::setDebugLocalScale(const glm::vec3 &scale) { applyLocalScale(scale); }

void SceneNode::setDebugName(std::string name) { debugName_ = std::move(name); }

std::string_view SceneNode::getDebugName() const { return debugName_; }

void SceneNode::setDebugTransformOverrideEnabled(bool enabled) {
  debugTransformOverrideEnabled_ = enabled;
}

void SceneNode::update(const FrameContext &ctx) {
  if (parent) {
    updateTransforms(parent->getWorldTransform());
  } else {
    updateTransforms(glm::mat4(1.0f));
  }

  for (auto &child : children) {
    child->update(ctx);
  }
}

void SceneNode::fixedUpdate(const FrameContext &ctx) {
  for (auto &child : children) {
    child->fixedUpdate(ctx);
  }
}

void SceneNode::render(const FrameContext &ctx) {

  for (auto &component : renderComponents_) {
    component->render(worldTransform, ctx);
  }

  for (auto &child : children) {
    child->render(ctx);
  }
}

glm::mat4 SceneNode::getWorldTransform() { return worldTransform; }

glm::vec3 SceneNode::getWorldPosition() {
  return glm::vec3(getWorldTransform()[3]);
}

glm::quat SceneNode::getWorldRotation() {
  glm::mat4 rotationMatrix = getWorldTransform();
  const glm::vec3 scale = getWorldScale();

  if (scale.x > 0.0f) {
    rotationMatrix[0] /= scale.x;
  }
  if (scale.y > 0.0f) {
    rotationMatrix[1] /= scale.y;
  }
  if (scale.z > 0.0f) {
    rotationMatrix[2] /= scale.z;
  }

  return glm::normalize(glm::quat_cast(rotationMatrix));
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
  child->setParent(this);
  children.push_back(std::move(child));
}

void SceneNode::setParent(SceneNode *parentNode) {
  parent = parentNode;
  markDirty();
}

void SceneNode::markDirty() {
  dirty = true;
  for (auto &child : children) {
    child->markDirty();
  }
}

void SceneNode::addRenderComponent(std::unique_ptr<VisualizerBase> component) {
  renderComponents_.push_back(std::move(component));
}

} // namespace DL
