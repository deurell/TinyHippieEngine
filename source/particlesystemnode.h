#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <random>
#include <vector>

class PlaneNode;

class ParticleSystemNode : public DL::SceneNode {
public:
  struct Config {
    int particleCount = 64;
    glm::vec3 gravity{0.0f, -32.0f, 0.0f};
    float drag = 0.6f;
    float speedMin = 14.0f;
    float speedMax = 26.0f;
    float directionZScale = 0.65f;
    float angularSpeedMin = glm::radians(-360.0f);
    float angularSpeedMax = glm::radians(360.0f);
    glm::vec3 startScale{0.25f, 0.25f, 0.25f};
    glm::vec3 endScale{0.02f, 0.02f, 0.02f};
    float lifetimeMin = 0.9f;
    float lifetimeMax = 1.6f;
    glm::vec4 startColorMin{0.75f, 0.35f, 0.05f, 1.0f};
    glm::vec4 startColorMax{1.0f, 0.85f, 0.3f, 1.0f};
    glm::vec4 endColor{0.3f, 0.08f, 0.02f, 1.0f};
  };

  explicit ParticleSystemNode(DL::IRenderDevice *renderDevice,
                              DL::Camera *camera,
                              DL::SceneNode *parentNode = nullptr);
  ParticleSystemNode(DL::IRenderDevice *renderDevice, DL::Camera *camera,
                     Config config, DL::SceneNode *parentNode);
  ~ParticleSystemNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;

  void resetParticles();
  void explode(const glm::vec3 &worldPosition);

private:
  struct ParticleState {
    PlaneNode *node = nullptr;
    glm::vec3 velocity{0.0f};
    glm::vec3 rotationAxis{0.0f, 1.0f, 0.0f};
    float angularSpeed = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    bool alive = false;
    glm::vec4 startColor{1.0f};
  };

  std::unique_ptr<PlaneNode> createParticleNode(const ParticleState &state);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::Camera *camera_ = nullptr;
  Config config_;
  std::mt19937 twister_;
  std::vector<ParticleState> particles_;
};
