//
// Created by Mikael Deurell on 2022-12-15.
//

#include "particlesystem.h"

namespace DL {

ParticleSystem::ParticleSystem() = default;

void ParticleSystem::addParticle(Particle &particle) {
  mParticles.emplace_back(particle);
}
void ParticleSystem::updatePhysics(float delta) {
  for(auto particle : mParticles) {
    particle.get().updatePhysics(delta);
  }
}

} // namespace DL