#pragma once

#include "physicsworld.h"
#include "scenenode.h"

namespace DL {

class PhysicsBodyComponent {
public:
  PhysicsBodyComponent(PhysicsWorld &physicsWorld, SceneNode &node,
                       const PhysicsBodyDesc &bodyDesc);
  ~PhysicsBodyComponent();

  PhysicsBodyComponent(const PhysicsBodyComponent &) = delete;
  PhysicsBodyComponent &operator=(const PhysicsBodyComponent &) = delete;

  void syncBeforeStep();
  void syncAfterStep();
  void setTransform(const glm::vec3 &position, const glm::quat &rotation);
  void setLinearVelocity(const glm::vec3 &velocity);
  [[nodiscard]] PhysicsBodyState state() const;
  [[nodiscard]] PhysicsBodyHandle handle() const { return handle_; }

private:
  PhysicsWorld *physicsWorld_ = nullptr;
  SceneNode *node_ = nullptr;
  PhysicsBodyHandle handle_;
  PhysicsBodyType bodyType_ = PhysicsBodyType::Static;
};

} // namespace DL
