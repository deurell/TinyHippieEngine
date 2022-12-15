//
// Created by Mikael Deurell on 2022-12-15.
//
#pragma once
#include "glm/vec3.hpp"
#include <vector>

namespace DL {

class Particle {
public:
  Particle(glm::vec3& visual, float mass, glm::vec3 gravity);

  void updatePhysics(float delta);
  void addForce(glm::vec3 force);
  void clearForces();
  void reset();

private:
  float mass;
  glm::vec3& visual;
  glm::vec3 gravity;
  glm::vec3 linearVelocity = {0, 0, 0};
  std::vector<glm::vec3> forces = {};
};

} // namespace DL
