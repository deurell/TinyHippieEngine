//
// Created by Mikael Deurell on 2022-12-15.
//

#include "particle.h"
#include <numeric>

DL::Particle::Particle(glm::vec3& visual, float mass, glm::vec3 gravity)
    : visual(visual), mass(mass), gravity(gravity) {}

void DL::Particle::updatePhysics(float delta) {
  if (mass == 0) {
    return;
  }
  forces.emplace_back(gravity * mass);
  glm::vec3 accumulatedForce = std::reduce(forces.begin(), forces.end());
  glm::vec3 acceleration = accumulatedForce / mass;
  linearVelocity += acceleration * delta;
  visual += linearVelocity * delta;
  forces.clear();
}
