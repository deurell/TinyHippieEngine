#pragma once

#include "camera.h"
#include "iscene.h"
#include "particle.h"
#include "plane.h"
#include "shader.h"
#include <string_view>
#include <vector>

class ParticleScene : public DL::IScene {
public:
  explicit ParticleScene(std::string_view glslVersionString);
  ~ParticleScene() override = default;

  void init() override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void updatePhysics(float delta);
  double mTime = 0;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::Plane> mPlane;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;
  glm::vec3 mLinearVelocity = {0, 0, 0};
  std::vector<glm::vec3> mForces = {};
  const glm::vec3 gravity = {0, -10.0f, 0};
  static constexpr float mass = 1.0;

  std::unique_ptr<DL::Particle> particle;
};
