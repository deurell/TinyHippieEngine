//
// Created by Mikael Deurell on 2022-12-15.
//

#pragma once
#include "particle.h"
#include <random>

namespace DL {

class ParticleSystem {
public:
  ParticleSystem();
  void addParticle(DL::Particle &particle);
  void updatePhysics(float delta);
  void explode(glm::vec3 position);
  void reset();

private:
  std::mt19937 twister_;
  std::vector<std::reference_wrapper<DL::Particle>> particles_ = {};
};

} // namespace DL
