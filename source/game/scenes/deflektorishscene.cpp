#include "deflektorishscene.h"

#include "game/deflektorish/deflektorishconfig.h"
#include "iscene.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr int kMaxBeamSegments = 28;
constexpr float kEpsilon = 0.001f;
constexpr float kManualRotateSpeed = 48.0f * 3.1415926535f / 180.0f;
constexpr float kReflektorPickRadius = 0.36f;
constexpr float kTargetPrepopDuration = 0.26f;
constexpr float kShakeStrength = 6.0f;
constexpr float kShakeMaxStrength = 18.0f;
constexpr float kShakeDecay = 1.65f;
constexpr float kShakeKickDecay = 28.0f;
constexpr int kStyleSource = 2;
constexpr int kStyleTarget = 3;
constexpr int kStyleBlocker = 4;
constexpr int kStyleReflectiveBlocker = 5;
constexpr int kStyleManualReflector = 6;
constexpr int kStyleAutoReflector = 7;
constexpr int kStyleSelection = 8;
constexpr int kStyleExplosion = 9;
constexpr int kStylePortal = 10;
constexpr int kStyleFilter = 11;
constexpr int kStyleSplitter = 12;
constexpr char kDefaultLevelPath[] =
    "Resources/Game/Deflektorish/Levels/level_01.json";

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
  updateFilters(ctx.delta_time);
  updateSelection(ctx.delta_time);

  BeamResult result = solveBeam();
  renderer_.updateBeamSegments(result);
  updateSource(ctx.delta_time, result);
  updateReflektorVisuals(ctx.delta_time, result);
  updateBlockerVisuals(ctx.delta_time, result);
  updatePortalVisuals(ctx.delta_time, result);
  updateFilterVisuals(ctx.delta_time, result);
  updateSplitterVisuals(ctx.delta_time, result);
  updateTargets(ctx.delta_time, result);
  updateExplosions(ctx.delta_time);

  SceneNode::update(ctx);
}

void DeflektorishScene::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void DeflektorishScene::onClick(double x, double y) {
  selectReflektorAtWorld(
      screenToWorld({static_cast<float>(x), static_cast<float>(y)}));
}

void DeflektorishScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
}

void DeflektorishScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
}

glm::vec2 DeflektorishScene::toWorld(glm::vec2 pixels) {
  return Deflektorish::gameToWorld(pixels);
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
  node->setLocalScale({halfSize.x * Deflektorish::kPixelToWorld,
                       halfSize.y * Deflektorish::kPixelToWorld, 1.0f});
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
  node->setOrthographicHeight(Deflektorish::kOrthographicHeight);
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
  const Deflektorish::LevelConfig level =
      Deflektorish::loadLevel(kDefaultLevelPath);
  grid_ = level.grid;
  sourcePosition_ = Deflektorish::cellToPosition(grid_, level.source.cell);
  sourceAngle_ = glm::radians(level.source.angleDegrees);
  source_ = addShaderPlane("source", kStyleSource, DL::BlendMode::Additive,
                           sourcePosition_, {41.0f, 41.0f}, 11, 0.10f,
                           sourceAngle_);

  renderer_.createBeamSegments(this, &cameraNode_->camera(), renderDevice_,
                               renderResourceCache_, kMaxBeamSegments);

  for (const Deflektorish::ReflektorConfig &config : level.reflektors) {
    Reflektor reflektor;
    reflektor.position = Deflektorish::cellToPosition(grid_, config.cell);
    reflektor.angle = glm::radians(config.angleDegrees);
    reflektor.automatic = config.automatic;
    reflektor.speed = config.speed;
    reflektor.node = addShaderPlane(
        config.automatic ? "reflektor_auto" : "reflektor_manual",
        config.automatic ? kStyleAutoReflector : kStyleManualReflector,
        DL::BlendMode::Alpha, reflektor.position, {22.0f, 5.0f}, 13, 0.12f,
        reflektor.angle);
    reflektors_.push_back(reflektor);
  }
  selectedReflektor_ = 0;
  selection_ =
      addShaderPlane("selection", kStyleSelection, DL::BlendMode::Alpha,
                     reflektors_[selectedReflektor_].position, {42.0f, 42.0f},
                     12, 0.10f);

  for (std::size_t i = 0; i < level.targets.size(); ++i) {
    Target target;
    target.position = Deflektorish::cellToPosition(grid_, level.targets[i].cell);
    target.phase = target.position.x * 0.071f + target.position.y * 0.113f;
    target.node = addShaderPlane("target_" + std::to_string(i + 1),
                                 kStyleTarget, DL::BlendMode::Alpha,
                                 target.position, {13.0f, 13.0f}, 8, 0.04f);
    targets_.push_back(target);
  }

  for (int i = 0; i < level.explosionPoolSize; ++i) {
    Explosion explosion;
    explosion.node = addShaderPlane("explosion_" + std::to_string(i + 1),
                                    kStyleExplosion, DL::BlendMode::Additive,
                                    {-10000.0f, -10000.0f}, {1.0f, 1.0f}, 18,
                                    0.16f);
    explosions_.push_back(explosion);
  }

  for (std::size_t i = 0; i < level.portals.size(); ++i) {
    const Deflektorish::PortalConfig &config = level.portals[i];
    Portal portal;
    portal.entryPosition =
        Deflektorish::cellToPosition(grid_, config.entryCell);
    portal.exitPosition = Deflektorish::cellToPosition(grid_, config.exitCell);
    portal.phase = config.phase;
    portal.entryNode =
        addShaderPlane("portal_entry_" + std::to_string(i + 1), kStylePortal,
                       DL::BlendMode::Additive, portal.entryPosition,
                       {27.0f, 27.0f}, 10, 0.11f);
    portal.exitNode =
        addShaderPlane("portal_exit_" + std::to_string(i + 1), kStylePortal,
                       DL::BlendMode::Additive, portal.exitPosition,
                       {27.0f, 27.0f}, 10, 0.11f);
    portals_.push_back(portal);
  }

  for (const Deflektorish::FilterConfig &config : level.filters) {
    Filter filter;
    filter.position = Deflektorish::cellToPosition(grid_, config.cell);
    filter.angle = glm::radians(config.angleDegrees);
    filter.automatic = config.automatic;
    filter.speed = config.speed;
    filter.node =
        addShaderPlane(config.automatic ? "angle_filter_auto" : "angle_filter",
                       kStyleFilter, DL::BlendMode::Alpha, filter.position,
                       {17.0f, 17.0f}, 7, 0.05f, filter.angle);
    filters_.push_back(filter);
  }

  for (std::size_t i = 0; i < level.splitters.size(); ++i) {
    const Deflektorish::SplitterConfig &config = level.splitters[i];
    Splitter splitter;
    splitter.position = Deflektorish::cellToPosition(grid_, config.cell);
    splitter.angle = glm::radians(config.angleDegrees);
    splitter.node = addShaderPlane("beam_splitter_" + std::to_string(i + 1),
                                   kStyleSplitter, DL::BlendMode::Alpha,
                                   splitter.position, {20.0f, 20.0f}, 8,
                                   0.06f, splitter.angle);
    splitters_.push_back(splitter);
  }

  for (const Deflektorish::BlockerConfig &config : level.blockers) {
    Blocker blocker;
    blocker.position = Deflektorish::cellToPosition(grid_, config.cell);
    blocker.reflective = config.reflective;
    blocker.node = addShaderPlane(
        config.reflective ? "reflective_blocker" : "solid_blocker",
        config.reflective ? kStyleReflectiveBlocker : kStyleBlocker,
        DL::BlendMode::Alpha, blocker.position, {16.0f, 16.0f},
        config.reflective ? 6 : 5, config.reflective ? 0.025f : 0.02f);
    blockers_.push_back(blocker);
  }
}

void DeflektorishScene::updateInput(const DL::FrameContext &ctx) {
  rotateInput_ = -std::clamp(ctx.input.moveAxis.x, -1.0f, 1.0f);
  const bool leftMouseDown = ctx.input.isMouseButtonDown(DL::MouseButton::Left);
  if (leftMouseDown && !previousLeftMouseDown_) {
    selectReflektorAtWorld(screenToWorld(ctx.input.sceneMousePosition));
  }
  previousLeftMouseDown_ = leftMouseDown;

  const bool selectNextDown = ctx.input.isActionDown(DL::Action::SelectNext);
  if (selectNextDown && !previousSelectNextDown_) {
    const int next = findNextManualReflektor(selectedReflektor_);
    if (next >= 0) {
      selectedReflektor_ = next;
      selectionFlash_ = 1.0f;
    }
  }
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

void DeflektorishScene::updateFilters(float dt) {
  for (std::size_t i = 0; i < filters_.size(); ++i) {
    Filter &filter = filters_[i];
    if (filter.automatic) {
      filter.angle += filter.speed * dt;
    }
    if (filter.node != nullptr) {
      filter.node->setLocalRotation(
          glm::quat(glm::vec3(0.0f, 0.0f, filter.angle)));
    }
  }
}

void DeflektorishScene::updateSelection(float dt) {
  selectionFlash_ = std::max(selectionFlash_ - dt * 4.5f, 0.0f);
  if (selection_ == nullptr) {
    return;
  }
  if (selectedReflektor_ < 0) {
    return;
  }
  const glm::vec2 world = toWorld(reflektors_[selectedReflektor_].position);
  renderer_.updateSelection(selection_, world, elapsed_, selectionFlash_);
}

DeflektorishScene::BeamResult DeflektorishScene::solveBeam() {
  Deflektorish::BeamWorld world;
  world.sourcePosition = sourcePosition_;
  world.sourceAngle = sourceAngle_;

  world.reflektors.reserve(reflektors_.size());
  for (const Reflektor &reflektor : reflektors_) {
    world.reflektors.push_back({reflektor.position, reflektor.angle});
  }

  world.targets.reserve(targets_.size());
  for (const Target &target : targets_) {
    world.targets.push_back({target.position, target.alive});
  }

  world.blockers.reserve(blockers_.size());
  for (const Blocker &blocker : blockers_) {
    world.blockers.push_back({blocker.position, blocker.reflective});
  }

  world.portals.reserve(portals_.size());
  for (const Portal &portal : portals_) {
    world.portals.push_back({portal.entryPosition, portal.exitPosition});
  }

  world.filters.reserve(filters_.size());
  for (const Filter &filter : filters_) {
    world.filters.push_back({filter.position, filter.angle});
  }

  world.splitters.reserve(splitters_.size());
  for (const Splitter &splitter : splitters_) {
    world.splitters.push_back({splitter.position, splitter.angle});
  }

  return Deflektorish::solveBeamWorld(world, renderer_.beamSegmentCapacity());
}

glm::vec2 DeflektorishScene::screenToWorld(glm::vec2 screenPosition) const {
  if (screenSize_.x <= 0.0f || screenSize_.y <= 0.0f) {
    return {0.0f, 0.0f};
  }

  const float aspect = screenSize_.x / screenSize_.y;
  const float halfHeight = Deflektorish::kOrthographicHeight * 0.5f;
  const float halfWidth = halfHeight * aspect;
  const glm::vec2 normalized{
      screenPosition.x / screenSize_.x,
      screenPosition.y / screenSize_.y,
  };
  const glm::vec2 cameraOffset =
      cameraNode_ != nullptr ? glm::vec2(cameraNode_->getLocalPosition())
                             : glm::vec2(0.0f);
  return {cameraOffset.x + (normalized.x - 0.5f) * halfWidth * 2.0f,
          cameraOffset.y + (0.5f - normalized.y) * halfHeight * 2.0f};
}

bool DeflektorishScene::selectReflektorAtWorld(glm::vec2 worldPosition) {
  int nearestIndex = -1;
  float nearestDistance = kReflektorPickRadius;

  for (std::size_t i = 0; i < reflektors_.size(); ++i) {
    const Reflektor &reflektor = reflektors_[i];
    if (reflektor.automatic) {
      continue;
    }
    const float distance =
        glm::length(toWorld(reflektor.position) - worldPosition);
    if (distance <= nearestDistance) {
      nearestDistance = distance;
      nearestIndex = static_cast<int>(i);
    }
  }

  if (nearestIndex < 0) {
    return false;
  }
  selectedReflektor_ = nearestIndex;
  selectionFlash_ = 1.0f;
  return true;
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
  renderer_.updateSource(source_, elapsed_, sourcePulse_, sourceLoad_);
}

void DeflektorishScene::updateReflektorVisuals(float dt,
                                               const BeamResult &result) {
  for (std::size_t i = 0; i < reflektors_.size(); ++i) {
    Reflektor &reflektor = reflektors_[i];
    renderer_.updateReflektor(
        reflektor.node, reflektor.glow, result.activeReflektors[i],
        reflektor.automatic, selectedReflektor_ == static_cast<int>(i),
        result.reflektorEnergy[i], dt);
  }
}

void DeflektorishScene::updateBlockerVisuals(float dt,
                                             const BeamResult &result) {
  for (std::size_t i = 0; i < blockers_.size(); ++i) {
    Blocker &blocker = blockers_[i];
    renderer_.updateBlocker(blocker.node, blocker.glow, blocker.energy,
                            blocker.hitPoint, result.activeBlockers[i],
                            result.blockerEnergy[i],
                            result.blockerHasHit[i], result.blockerHit[i],
                            dt);
  }
}

void DeflektorishScene::updatePortalVisuals(float dt,
                                            const BeamResult &result) {
  for (std::size_t i = 0; i < portals_.size(); ++i) {
    Portal &portal = portals_[i];
    renderer_.updatePortal(
        portal.entryNode, portal.exitNode, portal.glow, portal.entryHitPoint,
        portal.exitHitPoint, portal.phase, result.activePortals[i],
        result.portalEntryHit[i], result.portalExitHit[i],
        result.portalHasHit[i], elapsed_, dt);
  }
}

void DeflektorishScene::updateFilterVisuals(float dt,
                                            const BeamResult &result) {
  for (std::size_t i = 0; i < filters_.size(); ++i) {
    Filter &filter = filters_[i];
    renderer_.updateFilter(filter.node, filter.passGlow, filter.blockGlow,
                           filter.hitPoint, result.passingFilters[i],
                           result.blockedFilters[i], result.filterHasHit[i],
                           result.filterHit[i], dt);
  }
}

void DeflektorishScene::updateSplitterVisuals(float dt,
                                              const BeamResult &result) {
  for (std::size_t i = 0; i < splitters_.size(); ++i) {
    Splitter &splitter = splitters_[i];
    renderer_.updateSplitter(splitter.node, splitter.glow, splitter.hitPoint,
                             result.activeSplitters[i],
                             result.splitterHasHit[i],
                             result.splitterHit[i], dt);
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
        renderer_.hideNode(target.node);
      }
    }
    renderer_.updateTarget(target.node, target.alive, target.hitFlash,
                           elapsed_, target.phase);
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
  renderer_.showExplosion(it->node, position, energy);
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
      renderer_.updateExplosion(explosion.node, 1.0f, 0.0f, explosion.seed,
                                false);
      continue;
    }
    renderer_.updateExplosion(explosion.node, amount, explosion.energy,
                              explosion.seed, true);
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
  const glm::vec2 offset = offsetPixels * Deflektorish::kPixelToWorld;
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
