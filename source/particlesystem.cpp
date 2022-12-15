//
// Created by Mikael Deurell on 2022-12-15.
//

#include "particlesystem.h"
#include <random>

DL::ParticleSystem::ParticleSystem() = default;

void DL::ParticleSystem::addParticle(Particle &particle) {
  mParticles.emplace_back(particle);
}
void DL::ParticleSystem::updatePhysics(float delta) {
  for (auto particle : mParticles) {
    particle.get().updatePhysics(delta);
  }
}

void DL::ParticleSystem::explode() {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(-2000.0, 2000.0);

  for (auto particle : mParticles) {
    glm::vec3 force = {dist(mt), dist(mt), 0};
    particle.get().addForce(force);
  }
}
void DL::ParticleSystem::reset() {
  for (auto particle :  mParticles) {
    particle.get().reset();
  }
}
