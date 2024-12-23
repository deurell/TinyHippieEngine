#pragma once

#include "camera.h"
#include "iscene.h"
#include "particle.h"
#include "particlesystem.h"
#include "plane.h"
#include <string_view>
#include <vector>
#include <random>
//TODO:  evolve this to become an EffectsNode so that effects can spawn at any world position based on the node's transform.
class ParticleScene : public DL::IScene {
public:
  explicit ParticleScene(std::string_view glslVersionString);
  ~ParticleScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

  static constexpr int number_of_particles = 64;

private:
  std::mt19937 twister;
  double mTime = 0;
  std::unique_ptr<DL::Camera> mCamera;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;

  std::vector<std::unique_ptr<DL::Plane>> mPlanes = {};
  std::vector<std::unique_ptr<DL::Particle>> mParticles = {};
  std::unique_ptr<DL::ParticleSystem> particleSystem_;
  void initPlanes();
  void initParticles();
};
