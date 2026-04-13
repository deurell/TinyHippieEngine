#pragma once

#include "physicsworld.h"
#include <vector>

namespace DL {

class PhysicsBodyComponent;

class PhysicsContext {
public:
  PhysicsContext() = default;
  ~PhysicsContext() = default;

  PhysicsContext(const PhysicsContext &) = delete;
  PhysicsContext &operator=(const PhysicsContext &) = delete;
  PhysicsContext(PhysicsContext &&) noexcept = default;
  PhysicsContext &operator=(PhysicsContext &&) noexcept = default;

  void setGravity(const glm::vec3 &gravity);
  void registerBody(PhysicsBodyComponent &body);
  void unregisterBody(PhysicsBodyComponent &body);
  void step(float timeStep);
  [[nodiscard]] PhysicsRaycastHit raycast(
      const glm::vec3 &start, const glm::vec3 &end,
      unsigned short categoryMaskBits = 0xFFFF) const;
  void setDebugRenderingEnabled(bool enabled);
  [[nodiscard]] bool isDebugRenderingEnabled() const;
  void setDebugRenderSettings(const PhysicsDebugRenderSettings &settings);
  [[nodiscard]] PhysicsDebugRenderSettings getDebugRenderSettings() const;
  [[nodiscard]] std::vector<PhysicsDebugLine> getDebugLines() const;
  [[nodiscard]] PhysicsWorld &world() { return physicsWorld_; }
  [[nodiscard]] const PhysicsWorld &world() const { return physicsWorld_; }

private:
  PhysicsWorld physicsWorld_;
  std::vector<PhysicsBodyComponent *> bodies_;
};

} // namespace DL
