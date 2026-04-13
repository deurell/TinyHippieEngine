#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace DL {

enum class PhysicsBodyType {
  Static,
  Kinematic,
  Dynamic,
};

enum class PhysicsShapeType {
  Box,
  Sphere,
  Capsule,
};

struct PhysicsShapeDesc {
  PhysicsShapeType type = PhysicsShapeType::Box;
  glm::vec3 halfExtents{0.5f, 0.5f, 0.5f};
  float radius = 0.5f;
  float height = 1.0f;

  static PhysicsShapeDesc makeBox(const glm::vec3 &halfExtents);
  static PhysicsShapeDesc makeSphere(float radius);
  static PhysicsShapeDesc makeCapsule(float radius, float height);
};

struct PhysicsBodyDesc {
  PhysicsBodyType type = PhysicsBodyType::Static;
  PhysicsShapeDesc shape = PhysicsShapeDesc::makeBox({0.5f, 0.5f, 0.5f});
  glm::vec3 position{0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 linearVelocity{0.0f};
  float linearDamping = 0.05f;
  float angularDamping = 0.1f;
  unsigned short categoryBits = 0x0001;
  unsigned short maskBits = 0xFFFF;
};

struct PhysicsBodyState {
  glm::vec3 position{0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 linearVelocity{0.0f};
};

struct PhysicsBodyHandle {
  std::size_t value = 0;
  [[nodiscard]] bool valid() const { return value != 0; }
};

struct PhysicsRaycastHit {
  bool hasHit = false;
  PhysicsBodyHandle body;
  glm::vec3 point{0.0f};
  glm::vec3 normal{0.0f, 1.0f, 0.0f};
  float fraction = 1.0f;
};

class PhysicsWorld {
public:
  PhysicsWorld();
  ~PhysicsWorld();

  PhysicsWorld(PhysicsWorld &&) noexcept;
  PhysicsWorld &operator=(PhysicsWorld &&) noexcept;

  PhysicsWorld(const PhysicsWorld &) = delete;
  PhysicsWorld &operator=(const PhysicsWorld &) = delete;

  void setGravity(const glm::vec3 &gravity);
  PhysicsBodyHandle createBody(const PhysicsBodyDesc &desc);
  void destroyBody(PhysicsBodyHandle handle);
  void step(float timeStep);
  void setBodyTransform(PhysicsBodyHandle handle, const glm::vec3 &position,
                        const glm::quat &rotation);
  void setLinearVelocity(PhysicsBodyHandle handle, const glm::vec3 &velocity);
  [[nodiscard]] PhysicsBodyState getBodyState(PhysicsBodyHandle handle) const;
  [[nodiscard]] PhysicsRaycastHit raycast(
      const glm::vec3 &start, const glm::vec3 &end,
      unsigned short categoryMaskBits = 0xFFFF) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace DL
