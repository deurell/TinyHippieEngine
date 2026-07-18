#include "deflektorishrenderer.h"

#include "camera.h"
#include "game/deflektorish/deflektorishconfig.h"
#include "scenenode.h"
#include "shaderplanenode.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace Deflektorish {
namespace {

constexpr float kThickness = 48.0f;
constexpr float kBeamThicknessPerReflect = 6.0f;
constexpr float kEpsilon = 0.001f;
constexpr float kBlockerGlowSpeed = 8.0f;
constexpr float kReflektorGlowSpeed = 14.0f;
constexpr int kStyleBeam = 1;

float atan2Vec(glm::vec2 v) { return std::atan2(v.y, v.x); }

} // namespace

void Renderer::createBeamSegments(DL::SceneNode *parent, DL::Camera *camera,
                                  DL::IRenderDevice *renderDevice,
                                  DL::RenderResourceCache *renderResourceCache,
                                  int maxSegments) {
  beamSegments_.clear();
  if (parent == nullptr || camera == nullptr || renderDevice == nullptr ||
      renderResourceCache == nullptr) {
    return;
  }

  beamSegments_.reserve(static_cast<std::size_t>(maxSegments));
  for (int i = 0; i < maxSegments; ++i) {
    DL::ShaderPlaneNode::Config config;
    config.fragmentShader = "Shaders/deflektorish.frag";
    config.blendMode = DL::BlendMode::Additive;
    config.depthTest = false;
    config.proceduralStyle = kStyleBeam;
    auto node = std::make_unique<DL::ShaderPlaneNode>(
        std::move(config), parent, camera, renderDevice, renderResourceCache);
    node->setDebugName("beam_segment_" + std::to_string(i + 1));
    node->setRenderLayer(9);
    node->setLocalPosition({-100.0f, -100.0f, 0.0f});
    node->setLocalScale({0.001f, 0.001f, 1.0f});

    BeamSegmentNode segment;
    segment.node = node.get();
    parent->addChild(std::move(node));
    beamSegments_.push_back(segment);
  }
}

void Renderer::updateBeamSegments(const BeamSolveResult &result) {
  for (BeamSegmentNode &segment : beamSegments_) {
    hideSegment(segment);
  }

  const std::size_t count =
      std::min(result.segments.size(), beamSegments_.size());
  for (std::size_t i = 0; i < count; ++i) {
    const BeamSegment &solved = result.segments[i];
    layoutSegment(beamSegments_[i], solved.start, solved.end, solved.energy);
  }
}

std::size_t Renderer::beamSegmentCapacity() const {
  return beamSegments_.size();
}

void Renderer::updateSource(DL::ShaderPlaneNode *node, float elapsed,
                            float pulse, float load) {
  if (node != nullptr) {
    node->config.params0 = {elapsed, pulse, load, 0.0f};
  }
}

void Renderer::updateReflektor(DL::ShaderPlaneNode *node, float &glow,
                               bool active, bool automatic, bool selected,
                               float energy, float dt) {
  glow = approach(glow, active ? 1.0f : 0.0f, dt * kReflektorGlowSpeed);
  if (node != nullptr) {
    node->config.params0 = {glow, automatic ? 1.0f : 0.0f,
                            selected ? 1.0f : 0.0f, energy / 3.0f};
  }
}

void Renderer::updateBlocker(DL::ShaderPlaneNode *node, float &glow,
                             float &storedEnergy, glm::vec2 &hitPoint,
                             bool active, float energy, bool hasHit,
                             glm::vec2 hit, float dt) {
  glow = approach(glow, active ? 1.0f : 0.0f, dt * kBlockerGlowSpeed);
  storedEnergy = energy / 3.0f;
  if (hasHit) {
    hitPoint = hit;
  }
  if (node != nullptr) {
    node->config.params0 = {glow, storedEnergy, hitPoint.x, hitPoint.y};
  }
}

void Renderer::updatePortal(DL::ShaderPlaneNode *entryNode,
                            DL::ShaderPlaneNode *exitNode, float &glow,
                            glm::vec2 &entryHitPoint,
                            glm::vec2 &exitHitPoint, float phase, bool active,
                            glm::vec2 entryHit, glm::vec2 exitHit, bool hasHit,
                            float elapsed, float dt) {
  glow = approach(glow, active ? 1.0f : 0.0f, dt * 12.0f);
  if (hasHit) {
    entryHitPoint = entryHit;
    exitHitPoint = exitHit;
  }
  const glm::vec4 entryParams{elapsed, phase, glow, 0.0f};
  const glm::vec4 exitParams{elapsed, phase + 0.5f, glow, 1.0f};
  if (entryNode != nullptr) {
    entryNode->config.params0 = entryParams;
    entryNode->config.params1 = {entryHitPoint.x, entryHitPoint.y, 0.0f,
                                 0.0f};
  }
  if (exitNode != nullptr) {
    exitNode->config.params0 = exitParams;
    exitNode->config.params1 = {exitHitPoint.x, exitHitPoint.y, 0.0f, 0.0f};
  }
}

void Renderer::updateFilter(DL::ShaderPlaneNode *node, float &passGlow,
                            float &blockGlow, glm::vec2 &hitPoint,
                            bool passing, bool blocked, bool hasHit,
                            glm::vec2 hit, float dt) {
  passGlow = approach(passGlow, passing ? 1.0f : 0.0f, dt * 12.0f);
  blockGlow = approach(blockGlow, blocked ? 1.0f : 0.0f, dt * 14.0f);
  if (hasHit) {
    hitPoint = hit;
  }
  if (node != nullptr) {
    node->config.params0 = {passGlow, blockGlow, hitPoint.x, hitPoint.y};
  }
}

void Renderer::updateSplitter(DL::ShaderPlaneNode *node, float &glow,
                              glm::vec2 &hitPoint, bool active, bool hasHit,
                              glm::vec2 hit, float dt) {
  glow = approach(glow, active ? 1.0f : 0.0f, dt * 12.0f);
  if (hasHit) {
    hitPoint = hit;
  }
  if (node != nullptr) {
    node->config.params0 = {glow, hitPoint.x, hitPoint.y, 0.0f};
  }
}

void Renderer::updateTarget(DL::ShaderPlaneNode *node, bool alive,
                            float hitFlash, float elapsed, float phase) {
  if (node == nullptr) {
    return;
  }
  if (!alive) {
    hideNode(node);
    return;
  }
  const float swell = hitFlash * hitFlash * 0.32f;
  node->setLocalScale(
      {13.0f * kPixelToWorld * (1.0f + swell),
       13.0f * kPixelToWorld * (1.0f + swell), 1.0f});
  node->config.params0 = {elapsed * 1.8f, phase, hitFlash, 0.0f};
}

void Renderer::updateSelection(DL::ShaderPlaneNode *node,
                               glm::vec2 worldPosition, float elapsed,
                               float flash) {
  if (node == nullptr) {
    return;
  }
  node->setLocalPosition({worldPosition.x, worldPosition.y, 0.10f});
  node->config.params0 = {elapsed, flash, 0.0f, 0.0f};
}

void Renderer::hideNode(DL::ShaderPlaneNode *node) {
  if (node == nullptr) {
    return;
  }
  node->setLocalPosition({-100.0f, -100.0f, 0.0f});
  node->setLocalScale({0.001f, 0.001f, 1.0f});
}

void Renderer::showExplosion(DL::ShaderPlaneNode *node, glm::vec2 position,
                             float energy) {
  if (node == nullptr) {
    return;
  }
  const glm::vec2 world = toWorld(position);
  node->setLocalPosition({world.x, world.y, 0.16f});
  const float size =
      (118.0f + energy * 12.0f) * 1.55f * 0.5f * kPixelToWorld;
  node->setLocalScale({size, size, 1.0f});
}

void Renderer::updateExplosion(DL::ShaderPlaneNode *node, float amount,
                               float energy, float seed, bool active) {
  if (node == nullptr) {
    return;
  }
  if (!active) {
    hideNode(node);
    node->config.params0 = {1.0f, 0.0f, seed, 1.55f};
    return;
  }
  node->config.params0 = {amount, energy / 3.0f, seed, 1.55f};
}

void Renderer::layoutSegment(BeamSegmentNode &segment, glm::vec2 start,
                             glm::vec2 end, float energy) {
  if (segment.node == nullptr) {
    return;
  }
  const glm::vec2 delta = end - start;
  const float length = glm::length(delta);
  if (length <= kEpsilon) {
    hideSegment(segment);
    return;
  }
  const glm::vec2 center = start + delta * 0.5f;
  const glm::vec2 world = toWorld(center);
  segment.node->setLocalPosition({world.x, world.y, 0.07f});
  segment.node->setLocalRotation(
      glm::quat(glm::vec3(0.0f, 0.0f, atan2Vec(delta))));
  const float energizedThickness =
      kThickness + energy * kBeamThicknessPerReflect;
  segment.node->setLocalScale({length * 0.5f * kPixelToWorld,
                               energizedThickness * 0.5f * kPixelToWorld,
                               1.0f});
  segment.energy = energy;
  segment.node->config.params0 = {energy, length, 160.0f, 0.0f};
}

void Renderer::hideSegment(BeamSegmentNode &segment) {
  if (segment.node == nullptr) {
    return;
  }
  segment.node->setLocalPosition({-100.0f, -100.0f, 0.0f});
  segment.node->setLocalScale({0.001f, 0.001f, 1.0f});
  segment.energy = 0.0f;
  segment.node->config.params0 = {0.0f, 0.0f, 0.0f, 0.0f};
}

float Renderer::approach(float current, float target, float blend) {
  return current + (target - current) * std::clamp(blend, 0.0f, 1.0f);
}

glm::vec2 Renderer::toWorld(glm::vec2 pixels) {
  return gameToWorld(pixels);
}

} // namespace Deflektorish
