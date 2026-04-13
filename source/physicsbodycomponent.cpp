#include "physicsbodycomponent.h"

namespace DL {

PhysicsBodyComponent::PhysicsBodyComponent(PhysicsWorld &physicsWorld,
                                           SceneNode &node,
                                           const PhysicsBodyDesc &bodyDesc)
    : physicsWorld_(&physicsWorld), node_(&node), bodyType_(bodyDesc.type) {
  handle_ = physicsWorld_->createBody(bodyDesc);
}

PhysicsBodyComponent::~PhysicsBodyComponent() {
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
    node_->setLocalPosition(bodyState.position);
    node_->setLocalRotation(bodyState.rotation);
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
