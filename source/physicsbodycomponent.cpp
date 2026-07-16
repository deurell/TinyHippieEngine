#include "physicsbodycomponent.h"

namespace DL {

namespace {

glm::vec3 worldPositionToNodeLocal(SceneNode &node,
                                   const glm::vec3 &worldPosition) {
  SceneNode *parent = node.parentNode();
  if (parent == nullptr) {
    return worldPosition;
  }

  const glm::vec4 localPosition =
      glm::inverse(parent->getWorldTransform()) * glm::vec4(worldPosition, 1.0f);
  return glm::vec3(localPosition);
}

glm::quat worldRotationToNodeLocal(SceneNode &node,
                                   const glm::quat &worldRotation) {
  SceneNode *parent = node.parentNode();
  if (parent == nullptr) {
    return worldRotation;
  }

  return glm::normalize(glm::inverse(parent->getWorldRotation()) *
                        worldRotation);
}

} // namespace

PhysicsBodyComponent::PhysicsBodyComponent(PhysicsContext &physicsContext,
                                           SceneNode &node,
                                           const PhysicsBodyDesc &bodyDesc)
    : physicsContext_(&physicsContext), physicsWorld_(&physicsContext.world()),
      node_(&node), bodyType_(bodyDesc.type) {
  handle_ = physicsWorld_->createBody(bodyDesc);
  if (handle_.valid()) {
    physicsContext_->registerBody(*this);
  }
}

PhysicsBodyComponent::~PhysicsBodyComponent() {
  if (physicsContext_ != nullptr && handle_.valid()) {
    physicsContext_->unregisterBody(*this);
  }
  if (physicsWorld_ != nullptr && handle_.valid()) {
    physicsWorld_->destroyBody(handle_);
  }
}

void PhysicsBodyComponent::syncBeforeStep() {
  if (physicsWorld_ == nullptr || node_ == nullptr || !handle_.valid()) {
    return;
  }

  if (bodyType_ == PhysicsBodyType::Static ||
      bodyType_ == PhysicsBodyType::Kinematic) {
    physicsWorld_->setBodyTransform(handle_, node_->getWorldPosition(),
                                    node_->getWorldRotation());
  }
}

void PhysicsBodyComponent::syncAfterStep() {
  if (physicsWorld_ == nullptr || node_ == nullptr || !handle_.valid()) {
    return;
  }

  if (bodyType_ == PhysicsBodyType::Dynamic) {
    const auto bodyState = physicsWorld_->getBodyState(handle_);
    node_->setLocalPosition(worldPositionToNodeLocal(*node_, bodyState.position));
    node_->setLocalRotation(worldRotationToNodeLocal(*node_, bodyState.rotation));
  }
}

void PhysicsBodyComponent::setLinearVelocity(const glm::vec3 &velocity) {
  if (physicsWorld_ != nullptr && handle_.valid()) {
    physicsWorld_->setLinearVelocity(handle_, velocity);
  }
}

void PhysicsBodyComponent::setTransform(const glm::vec3 &position,
                                        const glm::quat &rotation) {
  if (physicsWorld_ != nullptr && handle_.valid()) {
    physicsWorld_->setBodyTransform(handle_, position, rotation);
  }
}

PhysicsBodyState PhysicsBodyComponent::state() const {
  if (physicsWorld_ == nullptr || !handle_.valid()) {
    return {};
  }
  return physicsWorld_->getBodyState(handle_);
}

} // namespace DL
