//
// Created by Mikael Deurell on 2022-12-15.
//

#pragma once
#include "particle.h"

namespace DL {

class ParticleSystem {
public:
  ParticleSystem();
  void addParticle(DL::Particle& particle);
  void updatePhysics(float delta);

private:
  std::vector<std::reference_wrapper<DL::Particle>> mParticles = {};
};

}
