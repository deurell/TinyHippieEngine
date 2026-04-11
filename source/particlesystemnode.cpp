#include "particlesystemnode.h"

#include "planenode.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

namespace {

glm::vec3 randomDirection(std::mt19937 &twister) {
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  glm::vec3 direction(0.0f);
  do {
    direction = {dist(twister), dist(twister), dist(twister) * 0.35f};
  } while (glm::length2(direction) < 0.0001f);

  return glm::normalize(direction);
}

glm::vec3 randomBurstDirection(std::mt19937 &twister, float zScale) {
  auto direction = randomDirection(twister);
  direction.z *= zScale;
  if (glm::length2(direction) < 0.0001f) {
    return {0.0f, 1.0f, 0.0f};
  }
  return glm::normalize(direction);
}

float randomRange(std::mt19937 &twister, float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(twister);
}

glm::vec4 randomColor(std::mt19937 &twister, const glm::vec4 &minColor,
                      const glm::vec4 &maxColor) {
  return {
      randomRange(twister, minColor.r, maxColor.r),
      randomRange(twister, minColor.g, maxColor.g),
      randomRange(twister, minColor.b, maxColor.b),
      randomRange(twister, minColor.a, maxColor.a),
  };
}

} // namespace

ParticleSystemNode::ParticleSystemNode(DL::IRenderDevice *renderDevice,
                                       DL::Camera *camera,
                                       DL::SceneNode *parentNode)
    : ParticleSystemNode(renderDevice, camera, Config{}, parentNode) {}

ParticleSystemNode::ParticleSystemNode(DL::IRenderDevice *renderDevice,
                                       DL::Camera *camera,
                                       Config config,
                                       DL::SceneNode *parentNode)
    : DL::SceneNode(parentNode), renderDevice_(renderDevice), camera_(camera),
      config_(config) {
  std::random_device device;
  twister_ = std::mt19937(device());
}

void ParticleSystemNode::init() {
  SceneNode::init();

  particles_.reserve(static_cast<std::size_t>(config_.particleCount));

  for (int i = 0; i < config_.particleCount; ++i) {
    ParticleState state;
    state.rotationAxis = randomDirection(twister_);
    state.angularSpeed = randomRange(twister_, config_.angularSpeedMin,
                                     config_.angularSpeedMax);

    auto particleNode = createParticleNode(state);
    state.node = particleNode.get();
    addChild(std::move(particleNode));
    particles_.push_back(state);
  }

  resetParticles();
}

void ParticleSystemNode::update(const DL::FrameContext &ctx) {
  for (auto &particle : particles_) {
    if (!particle.alive) {
      continue;
    }

    particle.age += ctx.delta_time;
    if (particle.age >= particle.lifetime) {
      particle.alive = false;
      particle.velocity = {0.0f, 0.0f, 0.0f};
      particle.node->setLocalScale({0.0f, 0.0f, 0.0f});
      particle.node->color = config_.endColor;
      continue;
    }

    particle.velocity += config_.gravity * ctx.delta_time;
    particle.velocity *= std::max(0.0f, 1.0f - config_.drag * ctx.delta_time);

    const auto position = particle.node->getLocalPosition();
    particle.node->setLocalPosition(position + particle.velocity * ctx.delta_time);

    const float t = particle.age / particle.lifetime;
    particle.node->setLocalScale(
        glm::mix(config_.startScale, config_.endScale, t));
    particle.node->color = glm::mix(particle.startColor, config_.endColor, t);

    const auto angle = static_cast<float>(ctx.total_time) * particle.angularSpeed;
    particle.node->setLocalRotation(glm::angleAxis(angle, particle.rotationAxis));
  }

  SceneNode::update(ctx);
}

void ParticleSystemNode::resetParticles() {
  for (auto &particle : particles_) {
    particle.alive = false;
    particle.age = 0.0f;
    particle.lifetime = config_.lifetimeMax;
    particle.velocity = {0.0f, 0.0f, 0.0f};
    particle.node->setLocalPosition({0.0f, 0.0f, 0.0f});
    particle.node->setLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    particle.node->setLocalScale({0.0f, 0.0f, 0.0f});
    particle.node->color = config_.endColor;
  }
}

void ParticleSystemNode::explode(const glm::vec3 &worldPosition) {
  for (auto &particle : particles_) {
    particle.alive = true;
    particle.age = 0.0f;
    particle.lifetime =
        randomRange(twister_, config_.lifetimeMin, config_.lifetimeMax);
    particle.startColor =
        randomColor(twister_, config_.startColorMin, config_.startColorMax);
    particle.node->setLocalPosition(worldPosition);
    particle.node->setLocalScale(config_.startScale);
    particle.node->color = particle.startColor;
    particle.velocity =
        randomBurstDirection(twister_, config_.directionZScale) *
        randomRange(twister_, config_.speedMin, config_.speedMax);
  }
}

std::unique_ptr<PlaneNode>
ParticleSystemNode::createParticleNode(const ParticleState &state) {
  auto particleNode =
      std::make_unique<PlaneNode>(this, camera_, renderDevice_);
  particleNode->planeType = PlaneNode::PlaneType::Simple;
  particleNode->init();
  particleNode->setLocalScale({0.0f, 0.0f, 0.0f});
  particleNode->color = config_.endColor;
  return particleNode;
}
