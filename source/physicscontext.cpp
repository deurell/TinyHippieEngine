#include "physicscontext.h"

#include "physicsbodycomponent.h"
#include <algorithm>

namespace DL {

void PhysicsContext::setGravity(const glm::vec3 &gravity) {
  physicsWorld_.setGravity(gravity);
}

void PhysicsContext::registerBody(PhysicsBodyComponent &body) {
  bodies_.push_back(&body);
}

void PhysicsContext::unregisterBody(PhysicsBodyComponent &body) {
  const auto it = std::remove(bodies_.begin(), bodies_.end(), &body);
  bodies_.erase(it, bodies_.end());
}

void PhysicsContext::step(float timeStep) {
  for (auto *body : bodies_) {
    if (body != nullptr) {
      body->syncBeforeStep();
    }
  }

  physicsWorld_.step(timeStep);

  for (auto *body : bodies_) {
    if (body != nullptr) {
      body->syncAfterStep();
    }
  }
}

PhysicsRaycastHit PhysicsContext::raycast(const glm::vec3 &start,
                                          const glm::vec3 &end,
                                          unsigned short categoryMaskBits) const {
  return physicsWorld_.raycast(start, end, categoryMaskBits);
}

} // namespace DL
