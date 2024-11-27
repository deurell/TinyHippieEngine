//
// Created by Mikael Deurell on 2022-12-15.
//

#include "particlesystem.h"

DL::ParticleSystem::ParticleSystem() {
  std::random_device device;
  twister = std::mt19937(device());
}

void DL::ParticleSystem::addParticle(Particle &particle) {
  mParticles.emplace_back(particle);
}

void DL::ParticleSystem::updatePhysics(float delta) {
  for (auto particle : mParticles) {
    particle.get().updatePhysics(delta);
  }
}

void DL::ParticleSystem::explode(glm::vec3 position) {
  std::uniform_real_distribution<double> dist(-500.0, 500.0);

  for (auto& particle : mParticles) {
    particle.get().setLinearVelocity({0,0,0});
    particle.get().setPosition(position);
    glm::vec3 force = {dist(twister), dist(twister)+500.0f, dist(twister)};
    particle.get().addForce(force);
  }
}

void DL::ParticleSystem::reset() {
  for (auto particle : mParticles) {
    particle.get().reset();
  }
}
