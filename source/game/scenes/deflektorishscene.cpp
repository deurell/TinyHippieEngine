#include "deflektorishscene.h"

#include "iscene.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr float kPixelToWorld = 0.01f;
constexpr glm::vec2 kScreenCenter{480.0f, 320.0f};
constexpr float kThickness = 48.0f;
constexpr float kBeamThicknessPerReflect = 6.0f;
constexpr float kMirrorLength = 44.0f;
constexpr float kTargetRadius = 14.0f;
constexpr float kBlockerHalfSize = 16.0f;
constexpr int kMaxBeamSegments = 16;
constexpr float kBeamRange = 2000.0f;
constexpr float kEpsilon = 0.001f;
constexpr float kHitGap = 6.0f;
constexpr float kManualRotateSpeed = 48.0f * 3.1415926535f / 180.0f;
constexpr float kTargetPrepopDuration = 0.26f;
constexpr float kBlockerGlowSpeed = 8.0f;
constexpr float kReflektorGlowSpeed = 14.0f;
constexpr float kShakeStrength = 6.0f;
constexpr float kShakeMaxStrength = 18.0f;
constexpr float kShakeDecay = 1.65f;
constexpr float kShakeKickDecay = 28.0f;
constexpr float kOrthographicHeight = 7.1f;
constexpr int kStyleBeam = 1;
constexpr int kStyleSource = 2;
constexpr int kStyleTarget = 3;
constexpr int kStyleBlocker = 4;
constexpr int kStyleReflectiveBlocker = 5;
constexpr int kStyleManualReflector = 6;
constexpr int kStyleAutoReflector = 7;
constexpr int kStyleSelection = 8;
constexpr int kStyleExplosion = 9;

float cross(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }

glm::vec2 direction(float angle) {
  return {std::cos(angle), std::sin(angle)};
}

float atan2Vec(glm::vec2 v) { return std::atan2(v.y, v.x); }

glm::vec2 reflectAcrossMirror(glm::vec2 ray, glm::vec2 mirrorDir) {
  const glm::vec2 normal = glm::normalize(glm::vec2{-mirrorDir.y, mirrorDir.x});
  return ray - normal * (2.0f * glm::dot(ray, normal));
}

glm::vec2 reflectFromNormal(glm::vec2 ray, glm::vec2 normal) {
  return ray - normal * (2.0f * glm::dot(ray, normal));
}

float approach(float current, float target, float blend) {
  return current + (target - current) * std::clamp(blend, 0.0f, 1.0f);
}

} // namespace

DeflektorishScene::DeflektorishScene(
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *renderResourceCache,
    std::function<void(glm::vec2, float)> postBumpCallback)
    : renderDevice_(renderDevice), renderResourceCache_(renderResourceCache),
      postBumpCallback_(std::move(postBumpCallback)) {}

void DeflektorishScene::init() {
  setDebugName("deflektorish_scene");

  createCameraNode();
  addBackground();
  spawnLevel();

  SceneNode::init();
  for (auto &child : children) {
    child->init();
  }
}

void DeflektorishScene::update(const DL::FrameContext &ctx) {
  elapsed_ += ctx.delta_time;
  updateInput(ctx);
  updateCameraShake(ctx.delta_time);
  updateReflektors(ctx.delta_time);
  updateSelection(ctx.delta_time);

  BeamResult result = solveBeam();
  updateSource(ctx.delta_time, result);
  updateReflektorVisuals(ctx.delta_time, result);
  updateBlockerVisuals(ctx.delta_time, result);
  updateTargets(ctx.delta_time, result);
  updateExplosions(ctx.delta_time);

  SceneNode::update(ctx);
}

void DeflektorishScene::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void DeflektorishScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
}

void DeflektorishScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
  onScreenSizeChanged(size);
}

glm::vec2 DeflektorishScene::grid(int x, int y) {
  return {x * 32.0f + 16.0f, y * 32.0f + 16.0f};
}

glm::vec2 DeflektorishScene::toWorld(glm::vec2 pixels) {
  return (pixels - kScreenCenter) * kPixelToWorld;
}

DL::ShaderPlaneNode *DeflektorishScene::addShaderPlane(
    std::string name, int proceduralStyle, DL::BlendMode blendMode,
    glm::vec2 position, glm::vec2 halfSize, int renderLayer, float z,
    float rotationRadians) {
  DL::ShaderPlaneNode::Config config;
  config.fragmentShader = "Shaders/deflektorish.frag";
  config.blendMode = blendMode;
  config.depthTest = false;
  config.proceduralStyle = proceduralStyle;
  auto node = std::make_unique<DL::ShaderPlaneNode>(
      std::move(config), this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  node->setDebugName(std::move(name));
  node->setRenderLayer(renderLayer);
  const glm::vec2 world = toWorld(position);
  node->setLocalPosition({world.x, world.y, z});
  node->setLocalScale({halfSize.x * kPixelToWorld, halfSize.y * kPixelToWorld,
                       1.0f});
  node->setLocalRotation(glm::quat(glm::vec3(0.0f, 0.0f, rotationRadians)));
  DL::ShaderPlaneNode *raw = node.get();
  addChild(std::move(node));
  return raw;
}

void DeflektorishScene::createCameraNode() {
  auto node = std::make_unique<DL::CameraNode>(this);
  node->setDebugName("main_camera");
  node->setActive(true);
  node->setProjection(DL::CameraProjection::Orthographic);
  node->setOrthographicHeight(kOrthographicHeight);
  node->setLocalPosition({0.0f, 0.0f, 10.5f});
  node->setLookAtTarget({0.0f, 0.0f, 0.0f});
  cameraNode_ = node.get();
  addChild(std::move(node));
}

void DeflektorishScene::addBackground() {
  DL::ShaderPlaneNode::Config backgroundConfig;
  backgroundConfig.blendMode = DL::BlendMode::Opaque;
  backgroundConfig.depthTest = true;

  auto back = std::make_unique<DL::ShaderPlaneNode>(
      backgroundConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  back->setDebugName("deflektorish_backplate");
  back->config.color = {0.025f, 0.030f, 0.047f, 1.0f};
  back->setRenderLayer(-30);
  back->setLocalPosition({0.0f, 0.0f, -0.08f});
  back->setLocalScale({6.8f, 4.8f, 1.0f});
  addChild(std::move(back));

  auto field = std::make_unique<DL::ShaderPlaneNode>(
      backgroundConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  field->setDebugName("deflektorish_playfield");
  field->config.color = {0.038f, 0.047f, 0.071f, 1.0f};
  field->setRenderLayer(-25);
  field->setLocalPosition({0.0f, 0.0f, -0.07f});
  field->setLocalScale({5.4f, 3.8f, 1.0f});
  addChild(std::move(field));
}

void DeflektorishScene::spawnLevel() {
  const std::vector<glm::ivec2> targets = {
      {6, 3},  {14, 3}, {21, 3}, {26, 4}, {4, 5},  {11, 5}, {18, 5},
      {24, 6}, {13, 7}, {20, 7}, {27, 7}, {5, 8},  {10, 9}, {16, 9},
      {23, 9}, {28, 10},{3, 11}, {11, 11},{15, 12},{24, 12},{6, 13},
      {13, 13},{20, 13},{27, 14},{4, 15}, {9, 15}, {17, 15},{25, 16},
      {7, 17}, {15, 17},{22, 17},{28, 18}};
  const std::vector<glm::ivec2> solid = {
      {4, 3},  {5, 3},  {4, 4},  {10, 4}, {11, 4}, {12, 4}, {12, 5},
      {20, 4}, {20, 5}, {21, 5}, {25, 5}, {26, 5}, {26, 6}, {7, 9},
      {8, 9},  {9, 9},  {8, 10}, {19, 8}, {19, 9}, {19, 10},{24, 10},
      {24, 11},{5, 14}, {6, 14}, {6, 15}, {14, 14},{14, 15},{15, 15},
      {23, 15},{24, 15},{23, 16}};
  const std::vector<glm::ivec2> reflect = {
      {18, 8}, {25, 10},{22, 15},{2, 12}, {2, 13}, {2, 14}, {2, 15},
      {2, 16}, {29, 6}, {29, 7}, {29, 8}, {29, 9}, {7, 2},  {8, 2},
      {9, 2},  {10, 2}, {15, 2}, {16, 2}, {17, 2}, {18, 2}, {28, 13},
      {28, 14},{28, 15},{11, 16},{12, 16},{13, 16},{16, 19},{17, 19},
      {18, 19},{19, 19},{20, 19}};

  source_ = addShaderPlane("source", kStyleSource, DL::BlendMode::Additive,
                           grid(3, 7), {41.0f, 41.0f}, 11, 0.10f);

  for (int i = 0; i < kMaxBeamSegments; ++i) {
    BeamSegment segment;
    segment.node = addShaderPlane("beam_segment_" + std::to_string(i + 1),
                                  kStyleBeam, DL::BlendMode::Additive,
                                  {-10000.0f, -10000.0f}, {1.0f, 1.0f}, 9,
                                  0.07f);
    segments_.push_back(segment);
  }

  auto addReflektor = [&](int x, int y, float degrees, bool automatic,
                          float speed) {
    Reflektor reflektor;
    reflektor.position = grid(x, y);
    reflektor.angle = glm::radians(degrees);
    reflektor.automatic = automatic;
    reflektor.speed = speed;
    reflektor.node = addShaderPlane(
        automatic ? "reflektor_auto" : "reflektor_manual",
        automatic ? kStyleAutoReflector : kStyleManualReflector,
        DL::BlendMode::Alpha, reflektor.position, {22.0f, 5.0f}, 13, 0.12f,
        reflektor.angle);
    reflektors_.push_back(reflektor);
  };
  addReflektor(12, 7, 45.0f, false, 0.0f);
  addReflektor(8, 12, 45.0f, true, 0.4f);
  addReflektor(18, 12, -45.0f, false, 0.0f);
  selectedReflektor_ = 0;
  selection_ =
      addShaderPlane("selection", kStyleSelection, DL::BlendMode::Alpha,
                     reflektors_[selectedReflektor_].position, {42.0f, 42.0f},
                     12, 0.10f);

  for (std::size_t i = 0; i < targets.size(); ++i) {
    Target target;
    target.position = grid(targets[i].x, targets[i].y);
    target.phase = target.position.x * 0.071f + target.position.y * 0.113f;
    target.node = addShaderPlane("target_" + std::to_string(i + 1),
                                 kStyleTarget, DL::BlendMode::Alpha,
                                 target.position, {13.0f, 13.0f}, 8, 0.04f);
    targets_.push_back(target);
  }

  for (int i = 0; i < 12; ++i) {
    Explosion explosion;
    explosion.node = addShaderPlane("explosion_" + std::to_string(i + 1),
                                    kStyleExplosion, DL::BlendMode::Additive,
                                    {-10000.0f, -10000.0f}, {1.0f, 1.0f}, 18,
                                    0.16f);
    explosions_.push_back(explosion);
  }

  auto addBlocker = [&](glm::ivec2 coord, bool reflective) {
    Blocker blocker;
    blocker.position = grid(coord.x, coord.y);
    blocker.reflective = reflective;
    blocker.node = addShaderPlane(
        reflective ? "reflective_blocker" : "solid_blocker",
        reflective ? kStyleReflectiveBlocker : kStyleBlocker,
        DL::BlendMode::Alpha, blocker.position, {16.0f, 16.0f}, reflective ? 6 : 5,
        reflective ? 0.025f : 0.02f);
    blockers_.push_back(blocker);
  };
  for (const glm::ivec2 &coord : solid) {
    addBlocker(coord, false);
  }
  for (const glm::ivec2 &coord : reflect) {
    addBlocker(coord, true);
  }
}

void DeflektorishScene::updateInput(const DL::FrameContext &ctx) {
  rotateInput_ = -std::clamp(ctx.input.moveAxis.x, -1.0f, 1.0f);
  const bool fireDown = ctx.input.isActionDown(DL::Action::Fire);
  const bool selectNextDown = ctx.input.isActionDown(DL::Action::SelectNext);
  if ((fireDown && !previousFireDown_) ||
      (selectNextDown && !previousSelectNextDown_)) {
    const int next = findNextManualReflektor(selectedReflektor_);
    if (next >= 0) {
      selectedReflektor_ = next;
      selectionFlash_ = 1.0f;
    }
  }
  previousFireDown_ = fireDown;
  previousSelectNextDown_ = selectNextDown;
}

void DeflektorishScene::updateReflektors(float dt) {
  for (std::size_t i = 0; i < reflektors_.size(); ++i) {
    Reflektor &reflektor = reflektors_[i];
    if (reflektor.automatic) {
      reflektor.angle += reflektor.speed * dt;
    } else if (selectedReflektor_ == static_cast<int>(i)) {
      reflektor.angle += rotateInput_ * kManualRotateSpeed * dt;
    }
    if (reflektor.node != nullptr) {
      reflektor.node->setLocalRotation(
          glm::quat(glm::vec3(0.0f, 0.0f, reflektor.angle)));
    }
  }
}

void DeflektorishScene::updateSelection(float dt) {
  selectionFlash_ = std::max(selectionFlash_ - dt * 4.5f, 0.0f);
  if (selection_ == nullptr || selectedReflektor_ < 0) {
    return;
  }
  const glm::vec2 world = toWorld(reflektors_[selectedReflektor_].position);
  selection_->setLocalPosition({world.x, world.y, 0.10f});
  selection_->config.params0 = {elapsed_, selectionFlash_, 0.0f, 0.0f};
}

namespace {

struct Hit {
  enum class Type { None, Reflektor, Target, Blocker };
  Type type = Type::None;
  float distance = std::numeric_limits<float>::max();
  glm::vec2 point{0.0f};
  int index = -1;
  glm::vec2 mirrorDir{0.0f};
  glm::vec2 normal{0.0f};
};

bool closer(const Hit &hit, const Hit &nearest) {
  return hit.type != Hit::Type::None && hit.distance < nearest.distance;
}

} // namespace

DeflektorishScene::BeamResult DeflektorishScene::solveBeam() {
  BeamResult result;
  result.activeReflektors.assign(reflektors_.size(), false);
  result.reflektorEnergy.assign(reflektors_.size(), 0.0f);
  result.activeBlockers.assign(blockers_.size(), false);
  result.blockerEnergy.assign(blockers_.size(), 0.0f);
  result.blockerHit.assign(blockers_.size(), glm::vec2(0.0f));
  result.blockerHasHit.assign(blockers_.size(), false);
  result.hitTargets.assign(targets_.size(), false);
  result.targetEnergy.assign(targets_.size(), 0.0f);

  glm::vec2 origin = grid(3, 7);
  glm::vec2 visualOrigin = origin;
  glm::vec2 rayDir = direction(0.0f);
  int ignoreReflektor = -1;
  int ignoreBlocker = -1;
  float energy = 0.0f;

  for (BeamSegment &segment : segments_) {
    Hit nearest;
    for (std::size_t i = 0; i < reflektors_.size(); ++i) {
      if (static_cast<int>(i) == ignoreReflektor) {
        continue;
      }
      const Reflektor &reflektor = reflektors_[i];
      const glm::vec2 mirrorDir = direction(reflektor.angle);
      const glm::vec2 half = mirrorDir * (kMirrorLength * 0.5f);
      const glm::vec2 a = reflektor.position - half;
      const glm::vec2 b = reflektor.position + half;
      const glm::vec2 seg = b - a;
      const float denominator = cross(rayDir, seg);
      if (std::abs(denominator) < kEpsilon) {
        continue;
      }
      const glm::vec2 toSegment = a - origin;
      const float rayDistance = cross(toSegment, seg) / denominator;
      const float segmentAmount = cross(toSegment, rayDir) / denominator;
      if (rayDistance <= kEpsilon || segmentAmount < 0.0f ||
          segmentAmount > 1.0f) {
        continue;
      }
      Hit hit;
      hit.type = Hit::Type::Reflektor;
      hit.distance = rayDistance;
      hit.point = origin + rayDir * rayDistance;
      hit.mirrorDir = mirrorDir;
      hit.index = static_cast<int>(i);
      if (closer(hit, nearest)) {
        nearest = hit;
      }
    }

    for (std::size_t i = 0; i < targets_.size(); ++i) {
      const Target &target = targets_[i];
      if (!target.alive) {
        continue;
      }
      const glm::vec2 toTarget = target.position - origin;
      const float projected = glm::dot(toTarget, rayDir);
      if (projected <= kEpsilon) {
        continue;
      }
      const glm::vec2 closest = origin + rayDir * projected;
      const float distanceToRay = glm::length(target.position - closest);
      if (distanceToRay > kTargetRadius) {
        continue;
      }
      const float hitDistance =
          projected - std::sqrt(kTargetRadius * kTargetRadius -
                                distanceToRay * distanceToRay);
      if (hitDistance <= kEpsilon) {
        continue;
      }
      Hit hit;
      hit.type = Hit::Type::Target;
      hit.distance = hitDistance;
      hit.point = origin + rayDir * hitDistance;
      hit.index = static_cast<int>(i);
      if (closer(hit, nearest)) {
        nearest = hit;
      }
    }

    for (std::size_t i = 0; i < blockers_.size(); ++i) {
      if (static_cast<int>(i) == ignoreBlocker) {
        continue;
      }
      const Blocker &blocker = blockers_[i];
      const float minX = blocker.position.x - kBlockerHalfSize;
      const float maxX = blocker.position.x + kBlockerHalfSize;
      const float minY = blocker.position.y - kBlockerHalfSize;
      const float maxY = blocker.position.y + kBlockerHalfSize;
      float tMin = -1000000.0f;
      float tMax = 1000000.0f;
      bool hitAxisX = true;

      if (std::abs(rayDir.x) < kEpsilon) {
        if (origin.x < minX || origin.x > maxX) {
          continue;
        }
      } else {
        const float tx1 = (minX - origin.x) / rayDir.x;
        const float tx2 = (maxX - origin.x) / rayDir.x;
        const float txMin = std::min(tx1, tx2);
        if (txMin > tMin) {
          tMin = txMin;
          hitAxisX = true;
        }
        tMax = std::min(tMax, std::max(tx1, tx2));
      }

      if (std::abs(rayDir.y) < kEpsilon) {
        if (origin.y < minY || origin.y > maxY) {
          continue;
        }
      } else {
        const float ty1 = (minY - origin.y) / rayDir.y;
        const float ty2 = (maxY - origin.y) / rayDir.y;
        const float tyMin = std::min(ty1, ty2);
        if (tyMin > tMin) {
          tMin = tyMin;
          hitAxisX = false;
        }
        tMax = std::min(tMax, std::max(ty1, ty2));
      }

      if (tMax < tMin || tMax <= kEpsilon) {
        continue;
      }
      Hit hit;
      hit.type = Hit::Type::Blocker;
      hit.distance = std::max(tMin, kEpsilon);
      hit.point = origin + rayDir * hit.distance;
      hit.index = static_cast<int>(i);
      hit.normal = hitAxisX
                       ? glm::vec2(rayDir.x > 0.0f ? -1.0f : 1.0f, 0.0f)
                       : glm::vec2(0.0f, rayDir.y > 0.0f ? -1.0f : 1.0f);
      if (closer(hit, nearest)) {
        nearest = hit;
      }
    }

    if (nearest.type == Hit::Type::Reflektor) {
      const float stopGap = std::min(kHitGap, nearest.distance * 0.5f);
      const glm::vec2 reflected = reflectAcrossMirror(rayDir, nearest.mirrorDir);
      result.activeReflektors[nearest.index] = true;
      result.reflektorEnergy[nearest.index] = energy;
      layoutSegment(segment, visualOrigin, nearest.point - rayDir * stopGap,
                    energy);
      origin = nearest.point + reflected * kHitGap;
      visualOrigin = nearest.point;
      rayDir = glm::normalize(reflected);
      ignoreReflektor = nearest.index;
      ignoreBlocker = -1;
      energy = std::min(energy + 1.0f, 3.0f);
    } else if (nearest.type == Hit::Type::Target) {
      layoutSegment(segment, visualOrigin, nearest.point, energy);
      result.hitTargets[nearest.index] = true;
      result.targetEnergy[nearest.index] = energy;
      for (BeamSegment &remaining : segments_) {
        if (&remaining > &segment) {
          hideSegment(remaining);
        }
      }
      return result;
    } else if (nearest.type == Hit::Type::Blocker) {
      Blocker &blocker = blockers_[nearest.index];
      layoutSegment(segment, visualOrigin, nearest.point, energy);
      result.activeBlockers[nearest.index] = true;
      result.blockerEnergy[nearest.index] = energy;
      result.blockerHit[nearest.index] =
          (nearest.point - blocker.position) / kBlockerHalfSize;
      result.blockerHasHit[nearest.index] = true;
      if (blocker.reflective) {
        const glm::vec2 reflected = reflectFromNormal(rayDir, nearest.normal);
        origin = nearest.point + reflected * kHitGap;
        visualOrigin = nearest.point;
        rayDir = glm::normalize(reflected);
        ignoreReflektor = -1;
        ignoreBlocker = nearest.index;
      } else {
        for (BeamSegment &remaining : segments_) {
          if (&remaining > &segment) {
            hideSegment(remaining);
          }
        }
        return result;
      }
    } else {
      layoutSegment(segment, visualOrigin, visualOrigin + rayDir * kBeamRange,
                    energy);
      for (BeamSegment &remaining : segments_) {
        if (&remaining > &segment) {
          hideSegment(remaining);
        }
      }
      return result;
    }
  }
  return result;
}

void DeflektorishScene::layoutSegment(BeamSegment &segment, glm::vec2 start,
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
  const float energizedThickness = kThickness + energy * kBeamThicknessPerReflect;
  segment.node->setLocalScale({length * 0.5f * kPixelToWorld,
                               energizedThickness * 0.5f * kPixelToWorld,
                               1.0f});
  segment.energy = energy;
  segment.node->config.params0 = {energy, length, 160.0f, 0.0f};
}

void DeflektorishScene::hideSegment(BeamSegment &segment) {
  if (segment.node == nullptr) {
    return;
  }
  segment.node->setLocalPosition({-100.0f, -100.0f, 0.0f});
  segment.node->setLocalScale({0.001f, 0.001f, 1.0f});
  segment.energy = 0.0f;
  segment.node->config.params0 = {0.0f, 0.0f, 0.0f, 0.0f};
}

void DeflektorishScene::updateSource(float dt, const BeamResult &result) {
  sourcePulse_ = std::max(sourcePulse_ - dt * 4.8f, 0.0f);
  float load = 0.0f;
  for (float energy : result.reflektorEnergy) {
    load = std::max(load, energy);
  }
  for (float energy : result.blockerEnergy) {
    load = std::max(load, energy);
  }
  sourceLoadTarget_ = load / 3.0f;
  sourceLoad_ = approach(sourceLoad_, sourceLoadTarget_, dt * 8.0f);
  if (source_ != nullptr) {
    source_->config.params0 = {elapsed_, sourcePulse_, sourceLoad_, 0.0f};
  }
}

void DeflektorishScene::updateReflektorVisuals(float dt,
                                               const BeamResult &result) {
  for (std::size_t i = 0; i < reflektors_.size(); ++i) {
    Reflektor &reflektor = reflektors_[i];
    const float target = result.activeReflektors[i] ? 1.0f : 0.0f;
    reflektor.glow = approach(reflektor.glow, target, dt * kReflektorGlowSpeed);
    if (reflektor.node != nullptr) {
      reflektor.node->config.params0 = {
          reflektor.glow, reflektor.automatic ? 1.0f : 0.0f,
          selectedReflektor_ == static_cast<int>(i) ? 1.0f : 0.0f,
          result.reflektorEnergy[i] / 3.0f};
    }
  }
}

void DeflektorishScene::updateBlockerVisuals(float dt,
                                             const BeamResult &result) {
  for (std::size_t i = 0; i < blockers_.size(); ++i) {
    Blocker &blocker = blockers_[i];
    const float target = result.activeBlockers[i] ? 1.0f : 0.0f;
    blocker.glow = approach(blocker.glow, target, dt * kBlockerGlowSpeed);
    blocker.energy = result.blockerEnergy[i] / 3.0f;
    if (result.blockerHasHit[i]) {
      blocker.hitPoint = result.blockerHit[i];
    }
    if (blocker.node != nullptr) {
      blocker.node->config.params0 = {blocker.glow, blocker.energy,
                                        blocker.hitPoint.x, blocker.hitPoint.y};
    }
  }
}

void DeflektorishScene::updateTargets(float dt, const BeamResult &result) {
  for (std::size_t i = 0; i < targets_.size(); ++i) {
    Target &target = targets_[i];
    if (result.hitTargets[i] && target.alive && target.dying <= 0.0f) {
      target.dying = kTargetPrepopDuration;
      target.hitEnergy = result.targetEnergy[i];
      target.hitFlash = 1.0f;
    }
    target.hitFlash = std::max(target.hitFlash - dt * 6.5f, 0.0f);
    if (target.dying > 0.0f) {
      target.dying -= dt;
      if (target.dying <= 0.0f) {
        target.alive = false;
        sourcePulse_ = std::min(sourcePulse_ + 0.35f + target.hitEnergy * 0.08f,
                                1.0f);
        if (postBumpCallback_) {
          postBumpCallback_(target.position, 1.0f + target.hitEnergy * 0.22f);
        }
        spawnExplosion(target.position, target.hitEnergy);
        startCameraShake(kShakeStrength + target.hitEnergy * 0.9f,
                         0.34f + target.hitEnergy * 0.025f);
        if (target.node != nullptr) {
          target.node->setLocalPosition({-100.0f, -100.0f, 0.0f});
        }
      }
    }
    if (target.alive && target.node != nullptr) {
      const float swell = target.hitFlash * target.hitFlash * 0.32f;
      target.node->setLocalScale({13.0f * kPixelToWorld * (1.0f + swell),
                                  13.0f * kPixelToWorld * (1.0f + swell),
                                  1.0f});
      target.node->config.params0 = {elapsed_ * 1.8f, target.phase,
                                       target.hitFlash, 0.0f};
    }
  }
}

void DeflektorishScene::spawnExplosion(glm::vec2 position, float energy) {
  auto it = std::find_if(explosions_.begin(), explosions_.end(),
                         [](const Explosion &explosion) {
                           return !explosion.active;
                         });
  if (it == explosions_.end()) {
    it = explosions_.begin();
  }
  it->active = true;
  it->position = position;
  it->time = 0.0f;
  it->duration = 0.92f + energy * 0.08f;
  it->energy = energy;
  it->seed = position.x * 0.037f + position.y * 0.071f + elapsed_ * 1.37f;
  if (it->node != nullptr) {
    const glm::vec2 world = toWorld(position);
    it->node->setLocalPosition({world.x, world.y, 0.16f});
    const float size = (118.0f + energy * 12.0f) * 1.55f * 0.5f * kPixelToWorld;
    it->node->setLocalScale({size, size, 1.0f});
  }
}

void DeflektorishScene::updateExplosions(float dt) {
  for (Explosion &explosion : explosions_) {
    if (!explosion.active || explosion.node == nullptr) {
      continue;
    }
    explosion.time += dt;
    const float amount = explosion.time / std::max(explosion.duration, kEpsilon);
    if (amount >= 1.0f) {
      explosion.active = false;
      explosion.node->setLocalPosition({-100.0f, -100.0f, 0.0f});
      explosion.node->setLocalScale({0.001f, 0.001f, 1.0f});
      explosion.node->config.params0 = {1.0f, 0.0f, explosion.seed, 1.55f};
      continue;
    }
    explosion.node->config.params0 = {
        amount, explosion.energy / 3.0f, explosion.seed, 1.55f};
  }
}

void DeflektorishScene::updateCameraShake(float dt) {
  if (cameraNode_ == nullptr) {
    return;
  }
  if (shakeTrauma_ <= 0.0f && shakeKick_ <= 0.0f) {
    cameraNode_->setLocalPosition({0.0f, 0.0f, 10.5f});
    cameraNode_->lookAtWorld({0.0f, 0.0f, 0.0f});
    return;
  }
  shakeTrauma_ = std::max(shakeTrauma_ - dt * kShakeDecay, 0.0f);
  shakeKick_ = std::max(shakeKick_ - dt * kShakeKickDecay, 0.0f);
  const float traumaAmount = kShakeMaxStrength * shakeTrauma_ * shakeTrauma_;
  const glm::vec2 offsetPixels{
      std::sin(elapsed_ * 86.0f + shakeSeed_) * traumaAmount +
          std::cos(shakeKickAngle_) * shakeKick_,
      std::sin(elapsed_ * 71.0f + shakeSeed_ * 1.7f) * traumaAmount +
          std::sin(shakeKickAngle_) * shakeKick_};
  const glm::vec2 offset = offsetPixels * kPixelToWorld;
  cameraNode_->setLocalPosition({offset.x, offset.y, 10.5f});
  cameraNode_->lookAtWorld({offset.x, offset.y, 0.0f});
}

void DeflektorishScene::startCameraShake(float strength, float /*duration*/) {
  shakeTrauma_ =
      std::min(shakeTrauma_ + strength / kShakeMaxStrength, 1.0f);
  shakeKick_ =
      std::min(shakeKick_ + strength * 0.28f, kShakeMaxStrength * 0.45f);
  shakeKickAngle_ = elapsed_ * 12.9898f;
}

int DeflektorishScene::findNextManualReflektor(int startIndex) const {
  if (reflektors_.empty()) {
    return -1;
  }
  for (std::size_t offset = 1; offset <= reflektors_.size(); ++offset) {
    const int index = (startIndex + static_cast<int>(offset)) %
                      static_cast<int>(reflektors_.size());
    if (!reflektors_[index].automatic) {
      return index;
    }
  }
  return -1;
}
