//
// Created by Mikael Deurell on 2022-12-15.
//

#include "particle.h"
#include <numeric>

DL::Particle::Particle(DL::Plane &visual, float mass, glm::vec3 gravity)
    : visual(visual), mass(mass), gravity(gravity) {}

void DL::Particle::updatePhysics(float delta) {
  if (mass == 0) {
    return;
  }
  forces.emplace_back(gravity * mass);
  glm::vec3 accumulatedForce = std::accumulate(forces.begin(), forces.end(), glm::vec3(0,0,0));
  glm::vec3 acceleration = accumulatedForce / mass;
  linearVelocity += acceleration * delta;
  visual.position += linearVelocity * delta;
  forces.clear();
}

void DL::Particle::addForce(glm::vec3 force) { forces.emplace_back(force); }

void DL::Particle::clearForces() { forces.clear(); }

void DL::Particle::reset() {
  forces.clear();
  linearVelocity = {0, 0, 0};
  visual.position = {0, 0, 0};
}
void DL::Particle::setPosition(glm::vec3 position) {
  visual.position = position;
}
void DL::Particle::setLinearVelocity(glm::vec3 velocity) {
  linearVelocity = velocity;
}
