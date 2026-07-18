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
constexpr float kShakeStrength = 5.2f;
constexpr float kShakeMaxStrength = 16.0f;
constexpr float kShakeDecay = 2.1f;
constexpr float kShakeKickDecay = 5.8f;
constexpr int kVictoryBlastCount = 10;
constexpr float kVictoryBlastInterval = 0.14f;
constexpr float kVictoryBlastStartDelay = 0.18f;
constexpr float kVictoryTextFadeDuration = 0.62f;
constexpr float kVictoryPostWaveDelay = 0.12f;
constexpr int kMinExplosionPoolSize = 28;
constexpr int kDebugVictoryKey = 86;
constexpr float kLevelParTimeSeconds = 90.0f;
constexpr int kMaxEnergyBonus = 5000;
constexpr float kTimeBonusPerSecond = 100.0f;
constexpr float kBonusLineDelay = 0.18f;
constexpr float kBonusScoreTickInterval = 0.17f;
constexpr float kBonusDoneHold = 3.0f;
constexpr float kBonusFadeDuration = 0.42f;
constexpr float kBonusFlashDecay = 4.2f;
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
constexpr int kStyleEnergyBar = 13;
constexpr int kStyleCompletionOverlay = 14;
constexpr char kDefaultLevelPath[] =
    "Resources/Game/Deflektorish/Levels/level_01.json";

float approach(float current, float target, float blend) {
  return current + (target - current) * std::clamp(blend, 0.0f, 1.0f);
}

std::string scoreLine(std::string_view label, int score) {
  std::string value = std::to_string(std::max(score, 0));
  while (value.size() < 6) {
    value.insert(value.begin(), '0');
  }
  return std::string(label) + "  " + value;
}

} // namespace

DeflektorishScene::DeflektorishScene(
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *renderResourceCache,
    std::function<void(glm::vec2, float)> postBumpCallback,
    std::function<void(Deflektorish::Sound, glm::vec2, float)> soundCallback)
    : renderDevice_(renderDevice), renderResourceCache_(renderResourceCache),
      postBumpCallback_(std::move(postBumpCallback)),
      soundCallback_(std::move(soundCallback)) {}

void DeflektorishScene::init() {
  setDebugName("deflektorish_scene");
  beamEnergy_.current = beamEnergyConfig_.maxEnergy;

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
  if (completionPhase_ == CompletionPhase::Playing) {
    levelElapsed_ += ctx.delta_time;
  }
  updateInput(ctx);
  updateCameraShake(ctx.delta_time);
  updateReflektors(ctx.delta_time);
  updateFilters(ctx.delta_time);
  updateSelection(ctx.delta_time);

  BeamResult result = solveBeam();
  updateBeamEnergy(ctx.delta_time, result);
  renderer_.updateBeamSegments(result);
  updateSource(ctx.delta_time, result);
  updateReflektorVisuals(ctx.delta_time, result);
  updateBlockerVisuals(ctx.delta_time, result);
  updatePortalVisuals(ctx.delta_time, result);
  updateFilterVisuals(ctx.delta_time, result);
  updateSplitterVisuals(ctx.delta_time, result);
  gameEvents_.clear();
  updateTargetState(ctx.delta_time, result);
  applyGameEvents();
  updateTargetVisuals();
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

void DeflektorishScene::onKey(int key) {
  if (key == kDebugVictoryKey &&
      completionPhase_ != CompletionPhase::Celebration &&
      completionPhase_ != CompletionPhase::FadeOut) {
    startVictoryCelebration();
  }
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
  energyBar_ = addShaderPlane("beam_energy_bar", kStyleEnergyBar,
                              DL::BlendMode::Alpha, {480.0f, 34.0f},
                              {176.0f, 10.0f}, 20, 0.20f);
  createCompletionOverlay();

  for (std::size_t i = 0; i < level.targets.size(); ++i) {
    Target target;
    target.position = Deflektorish::cellToPosition(grid_, level.targets[i].cell);
    target.phase = target.position.x * 0.071f + target.position.y * 0.113f;
    target.node = addShaderPlane("target_" + std::to_string(i + 1),
                                 kStyleTarget, DL::BlendMode::Alpha,
                                 target.position, {13.0f, 13.0f}, 8, 0.04f);
    targets_.push_back(target);
  }

  const int explosionPoolSize =
      std::max(level.explosionPoolSize, kMinExplosionPoolSize);
  for (int i = 0; i < explosionPoolSize; ++i) {
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

void DeflektorishScene::updateBeamEnergy(float dt, const BeamResult &result) {
  const Deflektorish::BeamHazards hazards =
      Deflektorish::analyzeBeamHazards(result, beamEnergyConfig_);
  Deflektorish::updateBeamEnergy(beamEnergy_, hazards, beamEnergyConfig_, dt);
  if (energyBar_ != nullptr) {
    const float ratio =
        beamEnergyConfig_.maxEnergy > 0.0f
            ? std::clamp(beamEnergy_.current / beamEnergyConfig_.maxEnergy,
                         0.0f, 1.0f)
            : 0.0f;
    energyBar_->config.params0 = {ratio, beamEnergy_.danger,
                                  beamEnergy_.drainPerSecond /
                                      std::max(beamEnergyConfig_.maxDrainPerSecond,
                                               0.001f),
                                  elapsed_};
  }
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
  const float lowEnergyLoad =
      beamEnergyConfig_.maxEnergy > 0.0f
          ? 1.0f - std::clamp(beamEnergy_.current / beamEnergyConfig_.maxEnergy,
                              0.0f, 1.0f)
          : 0.0f;
  sourceLoadTarget_ = std::max(load / 3.0f, lowEnergyLoad * 0.65f);
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

void DeflektorishScene::updateTargetState(float dt, const BeamResult &result) {
  for (std::size_t i = 0; i < targets_.size(); ++i) {
    Target &target = targets_[i];
    if (result.hitTargets[i] && target.alive && target.dying <= 0.0f) {
      target.dying = kTargetPrepopDuration;
      target.hitEnergy = result.targetEnergy[i];
      target.hitFlash = 1.0f;
      emitTargetFirstHit(i, target.position, target.hitEnergy);
    }
    target.hitFlash = std::max(target.hitFlash - dt * 6.5f, 0.0f);
    if (target.dying > 0.0f) {
      target.dying -= dt;
      if (target.dying <= 0.0f) {
        target.alive = false;
        emitTargetDestroyed(i, target.position, target.hitEnergy);
      }
    }
  }
}

void DeflektorishScene::updateTargetVisuals() {
  for (Target &target : targets_) {
    renderer_.updateTarget(target.node, target.alive, target.hitFlash,
                           elapsed_, target.phase);
  }
}

void DeflektorishScene::emitTargetFirstHit(std::size_t targetIndex,
                                           glm::vec2 position, float energy) {
  gameEvents_.push_back(
      {.type = GameEvent::Type::TargetFirstHit,
       .position = position,
       .energy = energy,
       .index = static_cast<int>(targetIndex)});
}

void DeflektorishScene::emitTargetDestroyed(std::size_t targetIndex,
                                            glm::vec2 position, float energy) {
  gameEvents_.push_back(
      {.type = GameEvent::Type::TargetDestroyed,
       .position = position,
       .energy = energy,
       .index = static_cast<int>(targetIndex)});
}

void DeflektorishScene::applyGameEvents() {
  for (const GameEvent &event : gameEvents_) {
    switch (event.type) {
    case GameEvent::Type::TargetFirstHit:
      applyTargetFirstHit(event);
      break;
    case GameEvent::Type::TargetDestroyed:
      applyTargetDestroyed(event);
      break;
    }
  }
}

void DeflektorishScene::applyTargetFirstHit(const GameEvent &event) {
  Deflektorish::addBeamEnergy(beamEnergy_, beamEnergyConfig_,
                              beamEnergyConfig_.targetHitEnergyGain);
  if (soundCallback_) {
    soundCallback_(Deflektorish::Sound::TargetFirstHit, event.position,
                   event.energy);
  }
}

void DeflektorishScene::applyTargetDestroyed(const GameEvent &event) {
  sourcePulse_ =
      std::min(sourcePulse_ + 0.35f + event.energy * 0.08f, 1.0f);
  if (postBumpCallback_) {
    postBumpCallback_(event.position, 1.0f + event.energy * 0.22f);
  }
  if (soundCallback_) {
    soundCallback_(Deflektorish::Sound::TargetDestroyed, event.position,
                   event.energy);
  }
  spawnExplosion(event.position, event.energy);
  startCameraShake(event.position, kShakeStrength + event.energy * 0.85f,
                   0.34f + event.energy * 0.025f);
  if (event.index >= 0 &&
      static_cast<std::size_t>(event.index) < targets_.size()) {
    renderer_.hideNode(targets_[static_cast<std::size_t>(event.index)].node);
  }
  if (!victoryCelebrationStarted_ && allTargetsDestroyed()) {
    startVictoryCelebration();
  }
}

void DeflektorishScene::spawnExplosion(glm::vec2 position, float energy) {
  auto it = std::find_if(explosions_.begin(), explosions_.end(),
                         [](const Explosion &explosion) {
                           return !explosion.active;
                         });
  if (it == explosions_.end()) {
    return;
  }
  it->active = true;
  it->position = position;
  it->time = 0.0f;
  it->duration = 1.34f + energy * 0.14f;
  it->energy = energy;
  it->seed = position.x * 0.037f + position.y * 0.071f + elapsed_ * 1.37f;
  renderer_.showExplosion(it->node, position, energy);
}

void DeflektorishScene::updateExplosions(float dt) {
  updateVictoryCelebration(dt);
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

bool DeflektorishScene::allTargetsDestroyed() const {
  return !targets_.empty() &&
         std::all_of(targets_.begin(), targets_.end(),
                     [](const Target &target) { return !target.alive; });
}

bool DeflektorishScene::anyExplosionActive() const {
  return std::any_of(explosions_.begin(), explosions_.end(),
                     [](const Explosion &explosion) {
                       return explosion.active;
                     });
}

float DeflektorishScene::victoryNoise(int index, float salt) const {
  const float value =
      std::sin((static_cast<float>(index) + 1.0f) * 37.719f +
               elapsed_ * 2.173f + salt * 19.113f) *
      43758.5453f;
  return value - std::floor(value);
}

glm::vec2 DeflektorishScene::victoryBlastPosition(int index) const {
  const float x = 105.0f + victoryNoise(index, 0.13f) * 750.0f;
  const float y = 95.0f + victoryNoise(index, 0.71f) * 470.0f;
  return {x, y};
}

void DeflektorishScene::createCompletionOverlay() {
  completionOverlay_ = addShaderPlane(
      "level_complete_overlay", kStyleCompletionOverlay, DL::BlendMode::Alpha,
      Deflektorish::kScreenCenter, {520.0f, 350.0f}, 21, 0.24f);
  if (completionOverlay_ != nullptr) {
    completionOverlay_->config.params0 = {0.0f, 0.0f, 0.0f, 0.0f};
  }

  auto title = std::make_unique<TextNode>(
      this, "LEVEL COMPLETE", renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  title->setDebugName("level_complete_title");
  title->setRenderLayer(22);
  title->setFontPixelHeight(44.0f);
  title->setTextAlignment(DL::TextAlignment::CENTER);
  title->setTextAnchor(DL::TextAnchor::CENTER);
  title->setTextColor({0.64f, 0.96f, 1.0f, 0.0f});
  title->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  title->setShadowOffset({2.0f, -2.0f});
  title->setLocalPosition({0.0f, 0.48f, 0.34f});
  title->setLocalScale({Deflektorish::kPixelToWorld,
                        Deflektorish::kPixelToWorld, 1.0f});
  completionTitle_ = title.get();
  addChild(std::move(title));

  auto subtitle = std::make_unique<TextNode>(
      this, "ALL TARGETS CLEARED", renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  subtitle->setDebugName("level_complete_subtitle");
  subtitle->setRenderLayer(22);
  subtitle->setFontPixelHeight(20.0f);
  subtitle->setTextAlignment(DL::TextAlignment::CENTER);
  subtitle->setTextAnchor(DL::TextAnchor::CENTER);
  subtitle->setTextColor({1.0f, 0.72f, 0.28f, 0.0f});
  subtitle->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  subtitle->setShadowOffset({1.4f, -1.4f});
  subtitle->setLocalPosition({0.0f, 0.08f, 0.34f});
  subtitle->setLocalScale({Deflektorish::kPixelToWorld,
                           Deflektorish::kPixelToWorld, 1.0f});
  completionSubtitle_ = subtitle.get();
  addChild(std::move(subtitle));

  auto bonusHeading = std::make_unique<TextNode>(
      this, "BONUS", renderDevice_, renderResourceCache_, &cameraNode_->camera());
  bonusHeading->setDebugName("bonus_heading");
  bonusHeading->setRenderLayer(22);
  bonusHeading->setFontPixelHeight(30.0f);
  bonusHeading->setTextAlignment(DL::TextAlignment::CENTER);
  bonusHeading->setTextAnchor(DL::TextAnchor::CENTER);
  bonusHeading->setTextColor({0.64f, 0.96f, 1.0f, 0.0f});
  bonusHeading->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  bonusHeading->setShadowOffset({1.8f, -1.8f});
  bonusHeading->setLocalPosition({0.0f, 0.48f, 0.35f});
  bonusHeading->setLocalScale({Deflektorish::kPixelToWorld,
                               Deflektorish::kPixelToWorld, 1.0f});
  bonusHeading_ = bonusHeading.get();
  addChild(std::move(bonusHeading));

  auto energy = std::make_unique<TextNode>(
      this, scoreLine("ENERGY BONUS", 0), renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  energy->setDebugName("bonus_energy");
  energy->setRenderLayer(22);
  energy->setFontPixelHeight(21.0f);
  energy->setTextAlignment(DL::TextAlignment::CENTER);
  energy->setTextAnchor(DL::TextAnchor::CENTER);
  energy->setTextColor({1.0f, 0.72f, 0.28f, 0.0f});
  energy->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  energy->setShadowOffset({1.4f, -1.4f});
  energy->setLocalPosition({0.0f, 0.14f, 0.35f});
  energy->setLocalScale({Deflektorish::kPixelToWorld,
                         Deflektorish::kPixelToWorld, 1.0f});
  bonusEnergy_ = energy.get();
  addChild(std::move(energy));

  auto time = std::make_unique<TextNode>(
      this, scoreLine("TIME BONUS", 0), renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  time->setDebugName("bonus_time");
  time->setRenderLayer(22);
  time->setFontPixelHeight(21.0f);
  time->setTextAlignment(DL::TextAlignment::CENTER);
  time->setTextAnchor(DL::TextAnchor::CENTER);
  time->setTextColor({1.0f, 0.72f, 0.28f, 0.0f});
  time->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  time->setShadowOffset({1.4f, -1.4f});
  time->setLocalPosition({0.0f, -0.12f, 0.35f});
  time->setLocalScale({Deflektorish::kPixelToWorld,
                       Deflektorish::kPixelToWorld, 1.0f});
  bonusTime_ = time.get();
  addChild(std::move(time));

  auto total = std::make_unique<TextNode>(
      this, scoreLine("TOTAL SCORE", 0), renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  total->setDebugName("bonus_total");
  total->setRenderLayer(22);
  total->setFontPixelHeight(25.0f);
  total->setTextAlignment(DL::TextAlignment::CENTER);
  total->setTextAnchor(DL::TextAnchor::CENTER);
  total->setTextColor({0.64f, 0.96f, 1.0f, 0.0f});
  total->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  total->setShadowOffset({1.7f, -1.7f});
  total->setLocalPosition({0.0f, -0.48f, 0.35f});
  total->setLocalScale({Deflektorish::kPixelToWorld,
                        Deflektorish::kPixelToWorld, 1.0f});
  bonusTotal_ = total.get();
  addChild(std::move(total));
}

void DeflektorishScene::startVictoryCelebration() {
  resetBonusTally();
  victoryCelebrationStarted_ = true;
  victoryCelebrationComplete_ = false;
  victoryBlastTimer_ = kVictoryBlastStartDelay;
  victoryBlastIndex_ = 0;
  victoryTime_ = 0.0f;
  victoryPostWaveTimer_ = kVictoryPostWaveDelay;
  completionFadeTime_ = 0.0f;
  completionPhase_ = CompletionPhase::Celebration;
  sourcePulse_ = 1.0f;
  startCameraShake(Deflektorish::kScreenCenter, kShakeStrength * 1.15f,
                   0.42f);
}

void DeflektorishScene::resetBonusTally() {
  bonusTallyPhase_ = BonusTallyPhase::Hidden;
  clearTime_ = 0.0f;
  bonusPhaseTime_ = 0.0f;
  bonusFadeTime_ = 0.0f;
  bonusScoreTickTimer_ = 0.0f;
  bonusEnergyFlash_ = 0.0f;
  bonusTimeFlash_ = 0.0f;
  bonusTotalFlash_ = 0.0f;
  energyBonus_ = 0;
  timeBonus_ = 0;
  totalBonus_ = 0;
  displayedEnergyBonus_ = 0;
  displayedTimeBonus_ = 0;
  displayedTotalBonus_ = 0;
  lastBonusHeadingText_.clear();
  lastBonusEnergyText_.clear();
  lastBonusTimeText_.clear();
  lastBonusTotalText_.clear();
  updateBonusText();
}

void DeflektorishScene::updateVictoryCelebration(float dt) {
  if (!victoryCelebrationStarted_ ||
      completionPhase_ == CompletionPhase::Playing) {
    return;
  }

  victoryTime_ += dt;
  if (completionPhase_ == CompletionPhase::Celebration) {
    if (victoryPostWaveTimer_ > 0.0f) {
      victoryPostWaveTimer_ -= dt;
      if (victoryPostWaveTimer_ <= 0.0f && postBumpCallback_) {
        postBumpCallback_(Deflektorish::kScreenCenter, 1.85f);
      }
    }

    victoryBlastTimer_ -= dt;
    while (victoryBlastIndex_ < kVictoryBlastCount &&
           victoryBlastTimer_ <= 0.0f) {
      const glm::vec2 position = victoryBlastPosition(victoryBlastIndex_);
      const float energy =
          2.35f + victoryNoise(victoryBlastIndex_, 1.31f) * 1.0f;
      spawnExplosion(position, energy);
      if (soundCallback_) {
        soundCallback_(Deflektorish::Sound::TargetDestroyed, position, energy);
      }
      startCameraShake(position, kShakeStrength * 0.34f + energy * 0.22f,
                       0.18f);
      ++victoryBlastIndex_;
      victoryBlastTimer_ += kVictoryBlastInterval;
    }
    if (victoryBlastIndex_ >= kVictoryBlastCount && !anyExplosionActive()) {
      completionPhase_ = CompletionPhase::FadeOut;
      completionFadeTime_ = 0.0f;
    }
  } else if (completionPhase_ == CompletionPhase::FadeOut) {
    completionFadeTime_ += dt;
    if (completionFadeTime_ >= kVictoryTextFadeDuration) {
      completionPhase_ = CompletionPhase::BonusPending;
      victoryCelebrationComplete_ = true;
      startBonusTally();
    }
  } else if (completionPhase_ == CompletionPhase::BonusPending) {
    updateBonusTally(dt);
  }

  updateCompletionOverlay();
}

void DeflektorishScene::updateCompletionOverlay() {
  if (!victoryCelebrationStarted_) {
    return;
  }

  const float intro = std::clamp(victoryTime_ / 0.44f, 0.0f, 1.0f);
  const float easedIntro = intro * intro * (3.0f - 2.0f * intro);
  const float blastProgress =
      std::clamp(static_cast<float>(victoryBlastIndex_) /
                     static_cast<float>(kVictoryBlastCount),
                 0.0f, 1.0f);
  const float pulse = 0.5f + 0.5f * std::sin(elapsed_ * 8.0f);
  float fade = 1.0f;
  if (completionPhase_ == CompletionPhase::FadeOut) {
    fade = 1.0f - std::clamp(completionFadeTime_ / kVictoryTextFadeDuration,
                             0.0f, 1.0f);
  } else if (completionPhase_ == CompletionPhase::BonusPending) {
    fade = 0.0f;
  }
  const float bonusBackdrop =
      completionPhase_ == CompletionPhase::BonusPending ? 0.34f : 1.0f;
  const float overlayAlpha =
      easedIntro * (0.58f + pulse * 0.08f) *
      std::max(fade, bonusBackdrop);

  if (completionOverlay_ != nullptr) {
    completionOverlay_->config.params0 = {victoryTime_, overlayAlpha,
                                          blastProgress, easedIntro};
    completionOverlay_->config.params1 = {fade, bonusBackdrop, 0.0f, 0.0f};
  }
  if (completionTitle_ != nullptr) {
    const float titleAlpha = std::clamp((victoryTime_ - 0.12f) / 0.36f,
                                        0.0f, 1.0f);
    completionTitle_->setTextColor(
        {0.64f + pulse * 0.12f, 0.96f, 1.0f, titleAlpha * fade});
    completionTitle_->setShadowColor(
        {0.0f, 0.02f, 0.05f, titleAlpha * fade * 0.72f});
  }
  if (completionSubtitle_ != nullptr) {
    const float subtitleAlpha = std::clamp((victoryTime_ - 0.46f) / 0.36f,
                                           0.0f, 1.0f);
    completionSubtitle_->setTextColor(
        {1.0f, 0.72f + pulse * 0.08f, 0.28f, subtitleAlpha * fade});
    completionSubtitle_->setShadowColor(
        {0.0f, 0.02f, 0.05f, subtitleAlpha * fade * 0.68f});
  }
}

void DeflektorishScene::startBonusTally() {
  if (bonusTallyPhase_ != BonusTallyPhase::Hidden) {
    return;
  }

  clearTime_ = levelElapsed_;
  const float energyRatio =
      beamEnergyConfig_.maxEnergy > 0.0f
          ? std::clamp(beamEnergy_.current / beamEnergyConfig_.maxEnergy,
                       0.0f, 1.0f)
          : 0.0f;
  energyBonus_ = static_cast<int>(
      std::round(energyRatio * static_cast<float>(kMaxEnergyBonus)));
  timeBonus_ = static_cast<int>(
      std::round(std::max(kLevelParTimeSeconds - clearTime_, 0.0f) *
                 kTimeBonusPerSecond));
  totalBonus_ = energyBonus_ + timeBonus_;
  displayedEnergyBonus_ = 0;
  displayedTimeBonus_ = 0;
  displayedTotalBonus_ = 0;
  bonusPhaseTime_ = 0.0f;
  bonusFadeTime_ = 0.0f;
  bonusScoreTickTimer_ = 0.0f;
  bonusTallyPhase_ = BonusTallyPhase::Energy;
  updateBonusText();
}

void DeflektorishScene::updateBonusTally(float dt) {
  if (bonusTallyPhase_ == BonusTallyPhase::Hidden ||
      bonusTallyPhase_ == BonusTallyPhase::Done) {
    updateBonusText();
    return;
  }

  bonusPhaseTime_ += dt;
  bonusScoreTickTimer_ = std::max(bonusScoreTickTimer_ - dt, 0.0f);
  bonusEnergyFlash_ = std::max(bonusEnergyFlash_ - dt * kBonusFlashDecay, 0.0f);
  bonusTimeFlash_ = std::max(bonusTimeFlash_ - dt * kBonusFlashDecay, 0.0f);
  bonusTotalFlash_ = std::max(bonusTotalFlash_ - dt * kBonusFlashDecay, 0.0f);
  if (bonusPhaseTime_ < kBonusLineDelay) {
    updateBonusText();
    return;
  }

  bool scoreAdvanced = false;
  const bool canScoreTick = bonusScoreTickTimer_ <= 0.0f;
  switch (bonusTallyPhase_) {
  case BonusTallyPhase::Hidden:
  case BonusTallyPhase::Done:
    break;
  case BonusTallyPhase::Hold:
    if (bonusPhaseTime_ >= kBonusDoneHold) {
      bonusTallyPhase_ = BonusTallyPhase::FadeOut;
      bonusFadeTime_ = 0.0f;
    }
    break;
  case BonusTallyPhase::FadeOut:
    bonusFadeTime_ += dt;
    if (bonusFadeTime_ >= kBonusFadeDuration) {
      bonusTallyPhase_ = BonusTallyPhase::Done;
    }
    break;
  case BonusTallyPhase::Energy: {
    const int previous = displayedEnergyBonus_;
    if (canScoreTick) {
      displayedEnergyBonus_ =
          advanceDisplayedScore(displayedEnergyBonus_, energyBonus_, false);
    }
    scoreAdvanced = displayedEnergyBonus_ != previous;
    if (displayedEnergyBonus_ >= energyBonus_ &&
        bonusPhaseTime_ >= kBonusLineDelay + 0.24f) {
      bonusTallyPhase_ = BonusTallyPhase::Time;
      bonusPhaseTime_ = 0.0f;
      bonusEnergyFlash_ = 1.0f;
    }
    break;
  }
  case BonusTallyPhase::Time: {
    const int previous = displayedTimeBonus_;
    if (canScoreTick) {
      displayedTimeBonus_ =
          advanceDisplayedScore(displayedTimeBonus_, timeBonus_, false);
    }
    scoreAdvanced = displayedTimeBonus_ != previous;
    if (displayedTimeBonus_ >= timeBonus_ &&
        bonusPhaseTime_ >= kBonusLineDelay + 0.24f) {
      bonusTallyPhase_ = BonusTallyPhase::Total;
      bonusPhaseTime_ = 0.0f;
      bonusTimeFlash_ = 1.0f;
    }
    break;
  }
  case BonusTallyPhase::Total: {
    const int previous = displayedTotalBonus_;
    if (canScoreTick) {
      displayedTotalBonus_ =
          advanceDisplayedScore(displayedTotalBonus_, totalBonus_, true);
    }
    scoreAdvanced = displayedTotalBonus_ != previous;
    if (displayedTotalBonus_ >= totalBonus_) {
      bonusTallyPhase_ = BonusTallyPhase::Hold;
      bonusPhaseTime_ = 0.0f;
      bonusTotalFlash_ = 1.0f;
    }
    break;
  }
  }

  if (scoreAdvanced && soundCallback_) {
    soundCallback_(Deflektorish::Sound::ScoreTick, Deflektorish::kScreenCenter,
                   0.0f);
    bonusScoreTickTimer_ = kBonusScoreTickInterval;
  }
  updateBonusText();
}

void DeflektorishScene::updateBonusText() {
  const bool visible = bonusTallyPhase_ != BonusTallyPhase::Hidden;
  const float pulse = 0.5f + 0.5f * std::sin(elapsed_ * 10.0f);
  float alpha = visible ? 1.0f : 0.0f;
  if (bonusTallyPhase_ == BonusTallyPhase::FadeOut) {
    alpha = 1.0f - std::clamp(bonusFadeTime_ / kBonusFadeDuration, 0.0f, 1.0f);
  } else if (bonusTallyPhase_ == BonusTallyPhase::Done) {
    alpha = 0.0f;
  }

  if (bonusHeading_ != nullptr) {
    bonusHeading_->setTextColor({0.64f + pulse * 0.12f, 0.96f, 1.0f, alpha});
    bonusHeading_->setShadowColor({0.0f, 0.02f, 0.05f, alpha * 0.72f});
  }

  const bool showEnergy = visible;
  const bool showTime = bonusTallyPhase_ == BonusTallyPhase::Time ||
                        bonusTallyPhase_ == BonusTallyPhase::Total ||
                        bonusTallyPhase_ == BonusTallyPhase::Hold ||
                        bonusTallyPhase_ == BonusTallyPhase::FadeOut ||
                        bonusTallyPhase_ == BonusTallyPhase::Done;
  const bool showTotal = bonusTallyPhase_ == BonusTallyPhase::Total ||
                         bonusTallyPhase_ == BonusTallyPhase::Hold ||
                         bonusTallyPhase_ == BonusTallyPhase::FadeOut ||
                         bonusTallyPhase_ == BonusTallyPhase::Done;

  const std::string energyText =
      scoreLine("ENERGY BONUS", displayedEnergyBonus_);
  const std::string timeText = scoreLine("TIME BONUS", displayedTimeBonus_);
  const std::string totalText = scoreLine("TOTAL SCORE", displayedTotalBonus_);

  if (bonusEnergy_ != nullptr) {
    if (lastBonusEnergyText_ != energyText) {
      bonusEnergy_->setText(energyText);
      lastBonusEnergyText_ = energyText;
    }
    const bool active = bonusTallyPhase_ == BonusTallyPhase::Energy;
    const float flash = std::max(active ? 0.55f + pulse * 0.35f : 0.0f,
                                 bonusEnergyFlash_);
    const glm::vec3 energyColor =
        glm::mix(glm::vec3(1.0f, 0.72f + pulse * 0.08f, 0.28f),
                 glm::vec3(0.64f + pulse * 0.18f, 0.96f, 1.0f),
                 std::clamp(flash, 0.0f, 1.0f));
    bonusEnergy_->setTextColor(
        {energyColor.r, energyColor.g, energyColor.b, showEnergy ? alpha : 0.0f});
    bonusEnergy_->setShadowColor(
        {0.0f, 0.02f, 0.05f, showEnergy ? alpha * 0.68f : 0.0f});
  }
  if (bonusTime_ != nullptr) {
    if (lastBonusTimeText_ != timeText) {
      bonusTime_->setText(timeText);
      lastBonusTimeText_ = timeText;
    }
    const bool active = bonusTallyPhase_ == BonusTallyPhase::Time;
    const float flash = std::max(active ? 0.55f + pulse * 0.35f : 0.0f,
                                 bonusTimeFlash_);
    const glm::vec3 timeColor =
        glm::mix(glm::vec3(1.0f, 0.72f + pulse * 0.08f, 0.28f),
                 glm::vec3(0.64f + pulse * 0.18f, 0.96f, 1.0f),
                 std::clamp(flash, 0.0f, 1.0f));
    bonusTime_->setTextColor(
        {timeColor.r, timeColor.g, timeColor.b, showTime ? alpha : 0.0f});
    bonusTime_->setShadowColor(
        {0.0f, 0.02f, 0.05f, showTime ? alpha * 0.68f : 0.0f});
  }
  if (bonusTotal_ != nullptr) {
    if (lastBonusTotalText_ != totalText) {
      bonusTotal_->setText(totalText);
      lastBonusTotalText_ = totalText;
    }
    const bool active = bonusTallyPhase_ == BonusTallyPhase::Total;
    const float flash = std::max(active ? 0.40f + pulse * 0.45f : 0.0f,
                                 bonusTotalFlash_);
    const glm::vec3 totalColor =
        glm::mix(glm::vec3(0.64f, 0.96f, 1.0f),
                 glm::vec3(1.0f, 0.82f + pulse * 0.14f, 0.32f),
                 std::clamp(flash, 0.0f, 1.0f));
    bonusTotal_->setTextColor(
        {totalColor.r, totalColor.g, totalColor.b, showTotal ? alpha : 0.0f});
    bonusTotal_->setShadowColor(
        {0.0f, 0.02f, 0.05f, showTotal ? alpha * 0.72f : 0.0f});
  }
}

int DeflektorishScene::advanceDisplayedScore(int current, int target,
                                             bool total) const {
  if (current >= target) {
    return target;
  }
  const int remaining = target - current;
  if (remaining <= 0) {
    return target;
  }

  const float progress =
      target > 0 ? std::clamp(static_cast<float>(current) /
                                  static_cast<float>(target),
                              0.0f, 1.0f)
                 : 1.0f;
  const float arcadeEnvelope =
      0.55f + std::sin(progress * 3.1415926535f) * (total ? 1.35f : 1.05f);
  const float desiredTicks = total ? 18.0f : 12.0f;
  const float rawStep =
      static_cast<float>(target) / desiredTicks * arcadeEnvelope;
  const int quantum = total ? 100 : 50;
  int step = static_cast<int>(std::round(rawStep / static_cast<float>(quantum))) *
             quantum;
  step = std::clamp(step, quantum, total ? 1400 : 700);
  if (remaining <= step + quantum) {
    return target;
  }
  return std::min(current + step, target);
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
  if (shakeKickDuration_ > 0.0f) {
    shakeKickTime_ = std::min(shakeKickTime_ + dt, shakeKickDuration_);
  }

  const float traumaAmount = kShakeMaxStrength * shakeTrauma_ * shakeTrauma_;
  const float rumbleX =
      std::sin(elapsed_ * 53.0f + shakeSeed_) * 0.65f +
      std::sin(elapsed_ * 31.0f + shakeSeed_ * 2.1f) * 0.35f;
  const float rumbleY =
      std::sin(elapsed_ * 47.0f + shakeSeed_ * 1.7f) * 0.65f +
      std::sin(elapsed_ * 29.0f + shakeSeed_ * 3.3f) * 0.35f;
  const float kickProgress =
      shakeKickDuration_ > 0.0f ? shakeKickTime_ / shakeKickDuration_ : 1.0f;
  const float kickEnvelope =
      (1.0f - std::clamp(kickProgress, 0.0f, 1.0f)) *
      (1.0f - std::clamp(kickProgress, 0.0f, 1.0f));
  const float kickPulse =
      std::sin(std::clamp(kickProgress, 0.0f, 1.0f) * 3.1415926535f);
  const glm::vec2 offsetPixels =
      glm::vec2(rumbleX, rumbleY) * traumaAmount +
      shakeKickDirection_ * shakeKick_ * kickEnvelope +
      glm::vec2(-shakeKickDirection_.y, shakeKickDirection_.x) * shakeKick_ *
          kickPulse * 0.24f;
  const glm::vec2 offset = offsetPixels * Deflektorish::kPixelToWorld;
  cameraNode_->setLocalPosition({offset.x, offset.y, 10.5f});
  cameraNode_->lookAtWorld({offset.x, offset.y, 0.0f});
}

void DeflektorishScene::startCameraShake(glm::vec2 position, float strength,
                                         float duration) {
  shakeTrauma_ =
      std::min(shakeTrauma_ + strength / (kShakeMaxStrength * 1.45f), 1.0f);
  shakeKick_ =
      std::min(shakeKick_ + strength * 0.52f, kShakeMaxStrength * 0.62f);
  shakeKickDuration_ = std::max(duration, 0.12f);
  shakeKickTime_ = 0.0f;
  const glm::vec2 fromCenter = position - Deflektorish::kScreenCenter;
  if (glm::length(fromCenter) > kEpsilon) {
    shakeKickDirection_ = -glm::normalize(fromCenter);
  } else {
    shakeKickDirection_ =
        glm::normalize(glm::vec2(std::cos(elapsed_ * 7.31f),
                                 std::sin(elapsed_ * 9.17f)));
  }
  shakeSeed_ = position.x * 0.017f + position.y * 0.031f + elapsed_ * 5.13f;
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
