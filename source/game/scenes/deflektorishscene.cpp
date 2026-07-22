#include "deflektorishscene.h"

#include "game/deflektorish/deflektorishconfig.h"
#include "game/deflektorish/deflektorishshaderparams.h"
#include "game/deflektorish/deflektorishshaderstyle.h"
#include "iscene.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr int kMaxBeamSegments = 28;
constexpr float kEpsilon = 0.001f;
constexpr float kManualRotateSpeed = 36.0f * 3.1415926535f / 180.0f;
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
constexpr int kBackgroundToggleKey = 66;
constexpr float kGameOverReturnDelay = 3.2f;
constexpr float kGameOverSkipDelay = 0.8f;
constexpr float kLevelParTimeSeconds = 90.0f;
constexpr int kMaxEnergyBonus = 5000;
constexpr float kTimeBonusPerSecond = 100.0f;
constexpr int kTargetClearScore = 75;
constexpr int kTargetBeamEnergyScore = 25;
constexpr float kHudSafePaddingPixels = 46.0f;
constexpr float kHudTopBandPixels = 36.0f;
constexpr float kBonusLineDelay = 0.09f;
constexpr float kBonusScoreTickInterval = 0.058f;
constexpr float kBonusDoneHold = 1.5f;
constexpr float kBonusFadeDuration = 0.21f;
constexpr float kBonusFlashDecay = 4.2f;
constexpr char kLevelRoot[] = "Resources/Game/Deflektorish/Levels/";
constexpr float kRoomSpacingPixels = 1120.0f;
constexpr float kRoomCameraPanDuration = 0.86f;
constexpr float kEntryTransitionDuration = 0.46f;
constexpr float kInactiveRoomAlpha = 0.18f;
constexpr float kAutoReflektorSpeedScale = 3.2f;
constexpr float kAutoFilterSpeedScale = 3.6f;
constexpr float kAutoReflektorWobble = 10.0f * 3.1415926535f / 180.0f;
constexpr float kAutoFilterWobble = 14.0f * 3.1415926535f / 180.0f;

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

std::string scoreDigits(int score) {
  std::string value = std::to_string(std::max(score, 0));
  while (value.size() < 8) {
    value.insert(value.begin(), '0');
  }
  return value;
}

} // namespace

DeflektorishScene::DeflektorishScene(
    DL::IRenderDevice *renderDevice, DL::RenderResourceCache *renderResourceCache,
    std::function<void(glm::vec2, float)> postBumpCallback,
    std::function<void(Deflektorish::Sound, glm::vec2, float)> soundCallback,
    std::function<void(int)> gameOverCallback)
    : renderDevice_(renderDevice), renderResourceCache_(renderResourceCache),
      postBumpCallback_(std::move(postBumpCallback)),
      soundCallback_(std::move(soundCallback)),
      gameOverCallback_(std::move(gameOverCallback)) {
  campaign_.loadDefaultLevelPaths(kLevelRoot, 10);
}

void DeflektorishScene::init() {
  setDebugName("deflektorish_scene");
  loadCampaign();
}

void DeflektorishScene::update(const DL::FrameContext &ctx) {
  elapsed_ += ctx.delta_time;
  updateCameraPan(ctx.delta_time);
  updateEntryTransition(ctx.delta_time);
  const bool gameplayActive = completionPhase_ == CompletionPhase::Playing &&
                              cameraPanDuration_ <= 0.0f;
  const bool fireDown = ctx.input.isActionDown(DL::Action::Fire);
  if (gameplayActive) {
    levelElapsed_ += ctx.delta_time;
    updateInput(ctx);
  } else {
    rotateInput_ = 0.0f;
    previousLeftMouseDown_ = false;
    previousSelectNextDown_ = false;
  }
  updateCameraShake(ctx.delta_time);
  updateHudPositions();
  if (gameplayActive) {
    updateReflektors(ctx.delta_time);
    updateFilters(ctx.delta_time);
  }
  updateSelection(ctx.delta_time);

  BeamResult result = gameplayActive ? solveBeam() : inactiveBeamResult();
  updateBeamEnergy(ctx.delta_time, result);
  if (gameplayActive && beamEnergy_.current <= 0.0f && !allTargetsDestroyed()) {
    startGameOver();
    result = inactiveBeamResult();
  }
  renderer_.updateBeamSegments(result);
  renderer_.updateBeamPulse(elapsed_, gameplayActive ? 0.66f : 0.0f,
                            gameplayActive ? beamDangerVisual_ : 0.0f);
  updateSource(ctx.delta_time, result);
  updateReflektorVisuals(ctx.delta_time, result);
  updateBlockerVisuals(ctx.delta_time, result);
  updatePortalVisuals(ctx.delta_time, result);
  updateFilterVisuals(ctx.delta_time, result);
  updateSplitterVisuals(ctx.delta_time, result);
  gameEvents_.clear();
  if (gameplayActive) {
    updateTargetState(ctx.delta_time, result);
  }
  applyGameEvents();
  updateTargetVisuals();
  updateExplosions(ctx.delta_time);
  updateGameOver(ctx.delta_time, fireDown);
  updateScoreHud(ctx.delta_time);
  updateParallaxBackgrounds();

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
  if (key == kBackgroundToggleKey) {
    setBackgroundEffectsEnabled(!backgroundEffectsEnabled_);
  }
  if (key == kDebugVictoryKey &&
      completionPhase_ != CompletionPhase::Celebration &&
      completionPhase_ != CompletionPhase::FadeOut &&
      completionPhase_ != CompletionPhase::GameOver) {
    startVictoryCelebration();
  }
}

void DeflektorishScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  for (RoomRuntime &room : rooms_) {
    room.orthographicHeight = fittedOrthographicHeight(room);
  }
  if (!rooms_.empty()) {
    cameraPanTargetHeight_ =
        rooms_[campaign_.currentLevelIndex()].orthographicHeight;
    if (cameraPanDuration_ <= 0.0f) {
      cameraBaseHeight_ = cameraPanTargetHeight_;
      if (cameraNode_ != nullptr) {
        cameraNode_->setOrthographicHeight(cameraBaseHeight_);
      }
    }
  }
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

void DeflektorishScene::addBackground(const RoomRuntime &room,
                                      std::size_t roomIndex) {
  DL::ShaderPlaneNode::Config flatConfig;
  flatConfig.fragmentShader = "Shaders/simple.frag";
  flatConfig.blendMode = DL::BlendMode::Opaque;
  flatConfig.depthTest = true;

  DL::ShaderPlaneNode::Config backgroundConfig;
  backgroundConfig.fragmentShader = "Shaders/deflektorish.frag";
  backgroundConfig.blendMode = DL::BlendMode::Opaque;
  backgroundConfig.depthTest = true;
  backgroundConfig.proceduralStyle =
      Deflektorish::shaderStyle(Deflektorish::ShaderStyle::ParallaxBackground);
  constexpr float kFieldPaddingPixels = 96.0f;
  constexpr float kBackPaddingPixels = 190.0f;
  const glm::vec2 contentCenterPixels =
      (room.boundsMinPixels + room.boundsMaxPixels) * 0.5f;
  const glm::vec2 contentSizePixels =
      room.boundsMaxPixels - room.boundsMinPixels;
  const glm::vec2 center =
      room.offsetPixels * Deflektorish::kPixelToWorld +
      Deflektorish::gameToWorld(contentCenterPixels);
  const glm::vec2 fieldHalfSize =
      glm::max((contentSizePixels + glm::vec2(kFieldPaddingPixels)) *
                   Deflektorish::kPixelToWorld * 0.5f,
               glm::vec2{5.4f, 3.8f});
  const glm::vec2 backHalfSize =
      glm::max((contentSizePixels + glm::vec2(kBackPaddingPixels)) *
                   Deflektorish::kPixelToWorld * 0.5f,
               glm::vec2{6.8f, 4.8f});

  auto flatBack = std::make_unique<DL::ShaderPlaneNode>(
      flatConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  flatBack->setDebugName("deflektorish_flat_backplate_" +
                         std::to_string(roomIndex + 1));
  flatBack->config.color = {0.025f, 0.030f, 0.047f, 1.0f};
  flatBack->setRenderLayer(-32);
  flatBack->setLocalPosition({center.x, center.y, -0.10f});
  flatBack->setLocalScale({backHalfSize.x, backHalfSize.y, 1.0f});
  flatBack->setVisible(!backgroundEffectsEnabled_);
  flatBackgrounds_.push_back(flatBack.get());
  addChild(std::move(flatBack));

  auto flatField = std::make_unique<DL::ShaderPlaneNode>(
      flatConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  flatField->setDebugName("deflektorish_flat_playfield_" +
                          std::to_string(roomIndex + 1));
  flatField->config.color = {0.038f, 0.047f, 0.071f, 1.0f};
  flatField->setRenderLayer(-31);
  flatField->setLocalPosition({center.x, center.y, -0.09f});
  flatField->setLocalScale({fieldHalfSize.x, fieldHalfSize.y, 1.0f});
  flatField->setVisible(!backgroundEffectsEnabled_);
  flatBackgrounds_.push_back(flatField.get());
  addChild(std::move(flatField));

  auto back = std::make_unique<DL::ShaderPlaneNode>(
      backgroundConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  back->setDebugName("deflektorish_backplate_" + std::to_string(roomIndex + 1));
  back->config.color = {1.0f, 1.0f, 1.0f, 1.0f};
  Deflektorish::setParallaxBackgroundParams(
      back.get(), elapsed_, cameraBaseWorld_, static_cast<float>(roomIndex));
  back->setRenderLayer(-30);
  back->setLocalPosition({center.x, center.y, -0.08f});
  back->setLocalScale({backHalfSize.x, backHalfSize.y, 1.0f});
  back->setVisible(backgroundEffectsEnabled_);
  parallaxBackgrounds_.push_back({back.get(), static_cast<float>(roomIndex)});
  addChild(std::move(back));

  auto field = std::make_unique<DL::ShaderPlaneNode>(
      backgroundConfig, this, &cameraNode_->camera(), renderDevice_,
      renderResourceCache_);
  field->setDebugName("deflektorish_playfield_" + std::to_string(roomIndex + 1));
  field->config.color = {1.0f, 1.0f, 1.0f, 1.0f};
  Deflektorish::setParallaxBackgroundParams(
      field.get(), elapsed_, cameraBaseWorld_, static_cast<float>(roomIndex));
  field->setRenderLayer(-25);
  field->setLocalPosition({center.x, center.y, -0.07f});
  field->setLocalScale({fieldHalfSize.x, fieldHalfSize.y, 1.0f});
  field->setVisible(backgroundEffectsEnabled_);
  parallaxBackgrounds_.push_back({field.get(), static_cast<float>(roomIndex)});
  addChild(std::move(field));
}

void DeflektorishScene::setBackgroundEffectsEnabled(bool enabled) {
  backgroundEffectsEnabled_ = enabled;
  for (DL::ShaderPlaneNode *node : flatBackgrounds_) {
    if (node != nullptr) {
      node->setVisible(!backgroundEffectsEnabled_);
    }
  }
  for (const auto &[node, roomIndex] : parallaxBackgrounds_) {
    (void)roomIndex;
    if (node != nullptr) {
      node->setVisible(backgroundEffectsEnabled_);
    }
  }
}

void DeflektorishScene::resetLevelRuntime() {
  children.clear();
  cameraNode_ = nullptr;
  source_ = nullptr;
  selection_ = nullptr;
  energyBar_ = nullptr;
  completionOverlay_ = nullptr;
  entryTransitionOverlay_ = nullptr;
  completionTitle_ = nullptr;
  completionSubtitle_ = nullptr;
  bonusHeading_ = nullptr;
  bonusEnergy_ = nullptr;
  bonusTime_ = nullptr;
  bonusTotal_ = nullptr;
  gameOverTitle_ = nullptr;
  gameOverScoreText_ = nullptr;
  scoreHud_ = nullptr;
  renderer_ = Deflektorish::Renderer{};
  rooms_.clear();
  activeReflektorIndices_.clear();
  activeTargetIndices_.clear();
  activeBlockerIndices_.clear();
  activePortalIndices_.clear();
  activeFilterIndices_.clear();
  activeSplitterIndices_.clear();
  reflektors_.clear();
  targets_.clear();
  blockers_.clear();
  explosions_.clear();
  portals_.clear();
  filters_.clear();
  splitters_.clear();
  gameEvents_.clear();
  parallaxBackgrounds_.clear();
  flatBackgrounds_.clear();
  selectedReflektor_ = -1;
  previousLeftMouseDown_ = false;
  previousSelectNextDown_ = false;
  previousGameOverFireDown_ = false;
  gameOverCallbackDispatched_ = false;
  rotateInput_ = 0.0f;
  sourcePulse_ = 0.0f;
  sourceLoad_ = 0.0f;
  sourceLoadTarget_ = 0.0f;
  beamEnergy_ = Deflektorish::BeamEnergyState{};
  beamEnergy_.current = beamEnergyConfig_.maxEnergy;
  selectionFlash_ = 0.0f;
  shakeTrauma_ = 0.0f;
  shakeKick_ = 0.0f;
  shakeKickDuration_ = 0.0f;
  shakeKickTime_ = 0.0f;
  shakeSeed_ = 1.7f;
  shakeKickDirection_ = {1.0f, 0.0f};
  victoryCelebrationStarted_ = false;
  victoryCelebrationComplete_ = false;
  victoryBlastIndex_ = 0;
  victoryBlastTimer_ = 0.0f;
  victoryTime_ = 0.0f;
  victoryPostWaveTimer_ = 0.0f;
  completionFadeTime_ = 0.0f;
  gameOverTime_ = 0.0f;
  gameOverRankFlash_ = 0.0f;
  gameOverScore_ = 0;
  entryTransition_ = Deflektorish::FadeTransition{};
  completionPhase_ = CompletionPhase::Playing;
  levelElapsed_ = 0.0f;
  campaign_.resetScore();
  displayedHudScore_ = 0;
  scoreHudPulse_ = 0.0f;
  scoreHudRollTime_ = 0.0f;
  lastScoreHudText_.clear();
  resetBonusTally();
}

void DeflektorishScene::loadCampaign() {
  if (!campaign_.hasLevels()) {
    return;
  }
  campaign_.setCurrentLevelIndex(0, 1);
  resetLevelRuntime();
  createCameraNode();
  renderer_.createBeamSegments(this, &cameraNode_->camera(), renderDevice_,
                               renderResourceCache_, kMaxBeamSegments);
  const auto &levelPaths = campaign_.levelPaths();
  for (std::size_t i = 0; i < levelPaths.size(); ++i) {
    const Deflektorish::LevelConfig level =
        Deflektorish::loadLevel(levelPaths[i]);
    const glm::vec2 offsetPixels{kRoomSpacingPixels * static_cast<float>(i),
                                 0.0f};
    spawnLevel(level, offsetPixels, i);
    addBackground(rooms_.back(), i);
  }
  selection_ = addShaderPlane(
      "selection", Deflektorish::shaderStyle(Deflektorish::ShaderStyle::Selection),
      DL::BlendMode::Alpha, {-10000.0f, -10000.0f}, {42.0f, 42.0f}, 12,
      0.10f);
  energyBar_ = addShaderPlane(
      "beam_energy_bar",
      Deflektorish::shaderStyle(Deflektorish::ShaderStyle::EnergyBar),
      DL::BlendMode::Alpha, {480.0f, 606.0f}, {150.0f, 8.0f}, 20, 0.20f);
  auto scoreHud = std::make_unique<TextNode>(
      this, scoreDigits(0), renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  scoreHud->setDebugName("score_hud");
  scoreHud->setRenderLayer(22);
  scoreHud->setFontPixelHeight(21.0f);
  scoreHud->setTextAlignment(DL::TextAlignment::CENTER);
  scoreHud->setTextAnchor(DL::TextAnchor::CENTER);
  scoreHud->setTextColor({0.72f, 0.98f, 1.0f, 0.92f});
  scoreHud->setShadowColor({0.0f, 0.02f, 0.05f, 0.78f});
  scoreHud->setShadowOffset({1.2f, -1.2f});
  scoreHud->setLocalScale({Deflektorish::kPixelToWorld,
                           Deflektorish::kPixelToWorld, 1.0f});
  scoreHud_ = scoreHud.get();
  addChild(std::move(scoreHud));
  createCompletionOverlay();
  createEntryTransitionOverlay();
  activateRoom(0, false);
  SceneNode::init();
  for (auto &child : children) {
    child->init();
  }
  if (screenSize_.x > 0.0f && screenSize_.y > 0.0f) {
    SceneNode::onScreenSizeChanged(screenSize_);
  }
  if (framebufferSize_.x > 0.0f && framebufferSize_.y > 0.0f) {
    onFramebufferSizeChanged(framebufferSize_);
  }
  updateHudPositions();
  updateEntryTransition(0.0f);
}

bool DeflektorishScene::isCurrentRoom(std::size_t roomIndex) const {
  return roomIndex == campaign_.currentLevelIndex();
}

void DeflektorishScene::rebuildActiveRoomMaps() {
  activeReflektorIndices_.clear();
  activeTargetIndices_.clear();
  activeBlockerIndices_.clear();
  activePortalIndices_.clear();
  activeFilterIndices_.clear();
  activeSplitterIndices_.clear();

  for (std::size_t i = 0; i < reflektors_.size(); ++i) {
    if (isCurrentRoom(reflektors_[i].roomIndex)) {
      activeReflektorIndices_.push_back(i);
    }
  }
  for (std::size_t i = 0; i < targets_.size(); ++i) {
    if (isCurrentRoom(targets_[i].roomIndex)) {
      activeTargetIndices_.push_back(i);
    }
  }
  for (std::size_t i = 0; i < blockers_.size(); ++i) {
    if (isCurrentRoom(blockers_[i].roomIndex)) {
      activeBlockerIndices_.push_back(i);
    }
  }
  for (std::size_t i = 0; i < portals_.size(); ++i) {
    if (isCurrentRoom(portals_[i].roomIndex)) {
      activePortalIndices_.push_back(i);
    }
  }
  for (std::size_t i = 0; i < filters_.size(); ++i) {
    if (isCurrentRoom(filters_[i].roomIndex)) {
      activeFilterIndices_.push_back(i);
    }
  }
  for (std::size_t i = 0; i < splitters_.size(); ++i) {
    if (isCurrentRoom(splitters_[i].roomIndex)) {
      activeSplitterIndices_.push_back(i);
    }
  }
}

void DeflektorishScene::activateRoom(std::size_t roomIndex, bool animated,
                                     bool preserveCompletionFlow) {
  if (rooms_.empty()) {
    return;
  }
  campaign_.setCurrentLevelIndex(roomIndex, rooms_.size());
  const RoomRuntime &room = rooms_[campaign_.currentLevelIndex()];
  sourcePosition_ = room.sourcePosition;
  sourceAngle_ = room.sourceAngle;
  source_ = room.sourceNode;
  rebuildActiveRoomMaps();
  selectedReflektor_ = findNextManualReflektor(-1);
  previousLeftMouseDown_ = false;
  previousSelectNextDown_ = false;
  rotateInput_ = 0.0f;
  sourcePulse_ = 0.0f;
  sourceLoad_ = 0.0f;
  sourceLoadTarget_ = 0.0f;
  beamEnergy_ = Deflektorish::BeamEnergyState{};
  beamEnergy_.current = beamEnergyConfig_.maxEnergy;
  selectionFlash_ = 1.0f;
  if (!preserveCompletionFlow) {
    victoryCelebrationStarted_ = false;
    victoryCelebrationComplete_ = false;
    completionFadeTime_ = 0.0f;
    completionPhase_ = CompletionPhase::Playing;
  }
  levelElapsed_ = 0.0f;
  if (!preserveCompletionFlow) {
    resetBonusTally();
  }

  cameraPanStartWorld_ = cameraBaseWorld_;
  cameraPanTargetWorld_ = room.cameraCenterWorld;
  cameraPanStartHeight_ =
      cameraBaseHeight_ > 0.0f ? cameraBaseHeight_ : room.orthographicHeight;
  cameraPanTargetHeight_ = room.orthographicHeight;
  cameraPanTime_ = 0.0f;
  cameraPanDuration_ = animated ? kRoomCameraPanDuration : 0.0f;
  if (!animated) {
    cameraBaseWorld_ = cameraPanTargetWorld_;
    cameraBaseHeight_ = cameraPanTargetHeight_;
    if (cameraNode_ != nullptr) {
      cameraNode_->setOrthographicHeight(cameraBaseHeight_);
    }
  }
  updateHudPositions();
  updateRoomVisibility();
}

void DeflektorishScene::updateRoomVisibility() {
  const auto roomColor = [&](std::size_t roomIndex) {
    if (isCurrentRoom(roomIndex)) {
      return glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
    }
    return glm::vec4{0.36f, 0.42f, 0.50f, kInactiveRoomAlpha};
  };

  for (std::size_t i = 0; i < rooms_.size(); ++i) {
    if (rooms_[i].sourceNode != nullptr) {
      rooms_[i].sourceNode->config.color = roomColor(i);
    }
  }
  for (Reflektor &reflektor : reflektors_) {
    if (reflektor.node != nullptr) {
      reflektor.node->config.color = roomColor(reflektor.roomIndex);
    }
  }
  for (Target &target : targets_) {
    if (target.node != nullptr) {
      target.node->config.color = roomColor(target.roomIndex);
    }
  }
  for (Blocker &blocker : blockers_) {
    if (blocker.node != nullptr) {
      blocker.node->config.color = roomColor(blocker.roomIndex);
    }
  }
  for (Portal &portal : portals_) {
    const glm::vec4 color = roomColor(portal.roomIndex);
    if (portal.entryNode != nullptr) {
      portal.entryNode->config.color = color;
    }
    if (portal.exitNode != nullptr) {
      portal.exitNode->config.color = color;
    }
  }
  for (Filter &filter : filters_) {
    if (filter.node != nullptr) {
      filter.node->config.color = roomColor(filter.roomIndex);
    }
  }
  for (Splitter &splitter : splitters_) {
    if (splitter.node != nullptr) {
      splitter.node->config.color = roomColor(splitter.roomIndex);
    }
  }
}

void DeflektorishScene::spawnLevel(const Deflektorish::LevelConfig &level,
                                   glm::vec2 offsetPixels,
                                   std::size_t roomIndex) {
  grid_ = level.grid;
  RoomRuntime room;
  room.name = level.name;
  room.offsetPixels = offsetPixels;
  room.cameraCenterWorld = offsetPixels * Deflektorish::kPixelToWorld;
  room.boundsMinPixels = {std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max()};
  room.boundsMaxPixels = {-std::numeric_limits<float>::max(),
                          -std::numeric_limits<float>::max()};
  room.sourcePosition =
      Deflektorish::cellToPosition(grid_, level.source.cell) + offsetPixels;
  room.sourceAngle = glm::radians(level.source.angleDegrees);
  room.sourceNode =
      addShaderPlane("source_" + std::to_string(roomIndex + 1),
                     Deflektorish::shaderStyle(Deflektorish::ShaderStyle::Source),
                     DL::BlendMode::Additive, room.sourcePosition,
                     {41.0f, 41.0f}, 11, 0.10f, room.sourceAngle);
  rooms_.push_back(room);
  auto includeBounds = [&](glm::vec2 positionPixels, float radiusPixels) {
    RoomRuntime &spawnedRoom = rooms_.back();
    const glm::vec2 local = positionPixels - offsetPixels;
    spawnedRoom.boundsMinPixels =
        glm::min(spawnedRoom.boundsMinPixels,
                 local - glm::vec2(radiusPixels));
    spawnedRoom.boundsMaxPixels =
        glm::max(spawnedRoom.boundsMaxPixels,
                 local + glm::vec2(radiusPixels));
  };
  includeBounds(room.sourcePosition, 48.0f);

  for (const Deflektorish::ReflektorConfig &config : level.reflektors) {
    Reflektor reflektor;
    reflektor.position =
        Deflektorish::cellToPosition(grid_, config.cell) + offsetPixels;
    reflektor.angle = glm::radians(config.angleDegrees);
    reflektor.baseAngle = reflektor.angle;
    reflektor.automatic = config.automatic;
    reflektor.speed = config.speed;
    reflektor.phase =
        reflektor.position.x * 0.017f + reflektor.position.y * 0.031f;
    reflektor.roomIndex = roomIndex;
    reflektor.node = addShaderPlane(
        config.automatic ? "reflektor_auto" : "reflektor_manual",
        Deflektorish::shaderStyle(config.automatic
                                      ? Deflektorish::ShaderStyle::AutoReflector
                                      : Deflektorish::ShaderStyle::ManualReflector),
        DL::BlendMode::Alpha, reflektor.position, {22.0f, 5.0f}, 13, 0.12f,
        reflektor.angle);
    reflektors_.push_back(reflektor);
    includeBounds(reflektor.position, 48.0f);
  }

  for (std::size_t i = 0; i < level.targets.size(); ++i) {
    Target target;
    target.position =
        Deflektorish::cellToPosition(grid_, level.targets[i].cell) +
        offsetPixels;
    target.roomIndex = roomIndex;
    target.phase = target.position.x * 0.071f + target.position.y * 0.113f;
    target.node = addShaderPlane("target_" + std::to_string(i + 1),
                                 Deflektorish::shaderStyle(
                                     Deflektorish::ShaderStyle::Target),
                                 DL::BlendMode::Alpha,
                                 target.position, {13.0f, 13.0f}, 8, 0.04f);
    targets_.push_back(target);
    includeBounds(target.position, 32.0f);
  }

  const int explosionPoolSize =
      std::max(level.explosionPoolSize, kMinExplosionPoolSize);
  for (int i = 0; i < explosionPoolSize; ++i) {
    Explosion explosion;
    explosion.node = addShaderPlane("explosion_" + std::to_string(i + 1),
                                    Deflektorish::shaderStyle(
                                        Deflektorish::ShaderStyle::Explosion),
                                    DL::BlendMode::Additive,
                                    {-10000.0f, -10000.0f}, {1.0f, 1.0f}, 18,
                                    0.16f);
    explosions_.push_back(explosion);
  }

  for (std::size_t i = 0; i < level.portals.size(); ++i) {
    const Deflektorish::PortalConfig &config = level.portals[i];
    Portal portal;
    portal.entryPosition =
        Deflektorish::cellToPosition(grid_, config.entryCell) + offsetPixels;
    portal.exitPosition =
        Deflektorish::cellToPosition(grid_, config.exitCell) + offsetPixels;
    portal.phase = config.phase;
    portal.roomIndex = roomIndex;
    portal.entryNode =
        addShaderPlane("portal_entry_" + std::to_string(i + 1),
                       Deflektorish::shaderStyle(
                           Deflektorish::ShaderStyle::Portal),
                       DL::BlendMode::Additive, portal.entryPosition,
                       {27.0f, 27.0f}, 10, 0.11f);
    portal.exitNode =
        addShaderPlane("portal_exit_" + std::to_string(i + 1),
                       Deflektorish::shaderStyle(
                           Deflektorish::ShaderStyle::Portal),
                       DL::BlendMode::Additive, portal.exitPosition,
                       {27.0f, 27.0f}, 10, 0.11f);
    portals_.push_back(portal);
    includeBounds(portal.entryPosition, 42.0f);
    includeBounds(portal.exitPosition, 42.0f);
  }

  for (const Deflektorish::FilterConfig &config : level.filters) {
    Filter filter;
    filter.position =
        Deflektorish::cellToPosition(grid_, config.cell) + offsetPixels;
    filter.angle = glm::radians(config.angleDegrees);
    filter.baseAngle = filter.angle;
    filter.automatic = config.automatic;
    filter.speed = config.speed;
    filter.phase = filter.position.x * 0.019f + filter.position.y * 0.037f;
    filter.roomIndex = roomIndex;
    filter.node =
        addShaderPlane(config.automatic ? "angle_filter_auto" : "angle_filter",
                       Deflektorish::shaderStyle(
                           Deflektorish::ShaderStyle::Filter),
                       DL::BlendMode::Alpha, filter.position, {17.0f, 17.0f},
                       7, 0.05f, filter.angle);
    filters_.push_back(filter);
    includeBounds(filter.position, 36.0f);
  }

  for (std::size_t i = 0; i < level.splitters.size(); ++i) {
    const Deflektorish::SplitterConfig &config = level.splitters[i];
    Splitter splitter;
    splitter.position =
        Deflektorish::cellToPosition(grid_, config.cell) + offsetPixels;
    splitter.angle = glm::radians(config.angleDegrees);
    splitter.roomIndex = roomIndex;
    splitter.node = addShaderPlane("beam_splitter_" + std::to_string(i + 1),
                                   Deflektorish::shaderStyle(
                                       Deflektorish::ShaderStyle::Splitter),
                                   DL::BlendMode::Alpha,
                                   splitter.position, {20.0f, 20.0f}, 8,
                                   0.06f, splitter.angle);
    splitters_.push_back(splitter);
    includeBounds(splitter.position, 42.0f);
  }

  for (const Deflektorish::BlockerConfig &config : level.blockers) {
    Blocker blocker;
    blocker.position =
        Deflektorish::cellToPosition(grid_, config.cell) + offsetPixels;
    blocker.reflective = config.reflective;
    blocker.roomIndex = roomIndex;
    blocker.node = addShaderPlane(
        config.reflective ? "reflective_blocker" : "solid_blocker",
        Deflektorish::shaderStyle(
            config.reflective ? Deflektorish::ShaderStyle::ReflectiveBlocker
                              : Deflektorish::ShaderStyle::Blocker),
        DL::BlendMode::Alpha, blocker.position, {16.0f, 16.0f},
        config.reflective ? 6 : 5, config.reflective ? 0.025f : 0.02f);
    blockers_.push_back(blocker);
    includeBounds(blocker.position, 34.0f);
  }
  RoomRuntime &spawnedRoom = rooms_.back();
  const glm::vec2 cameraBoundsMinPixels = spawnedRoom.boundsMinPixels;
  const glm::vec2 cameraBoundsMaxPixels =
      spawnedRoom.boundsMaxPixels + glm::vec2{0.0f, kHudTopBandPixels};
  const glm::vec2 contentCenterPixels =
      (cameraBoundsMinPixels + cameraBoundsMaxPixels) * 0.5f;
  spawnedRoom.cameraCenterWorld =
      spawnedRoom.offsetPixels * Deflektorish::kPixelToWorld +
      Deflektorish::gameToWorld(contentCenterPixels);
  spawnedRoom.orthographicHeight = fittedOrthographicHeight(spawnedRoom);
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
    if (!isCurrentRoom(reflektor.roomIndex)) {
      continue;
    }
    if (reflektor.automatic) {
      const float t = elapsed_ + reflektor.phase;
      const float wobble =
          std::sin(t * (2.1f + std::abs(reflektor.speed) * 1.7f)) *
          kAutoReflektorWobble;
      reflektor.angle =
          reflektor.baseAngle +
          elapsed_ * reflektor.speed * kAutoReflektorSpeedScale + wobble;
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
    if (!isCurrentRoom(filter.roomIndex)) {
      continue;
    }
    if (filter.automatic) {
      const float t = elapsed_ + filter.phase;
      const float wobble =
          std::sin(t * (2.8f + std::abs(filter.speed) * 1.5f)) *
          kAutoFilterWobble;
      filter.angle =
          filter.baseAngle + elapsed_ * filter.speed * kAutoFilterSpeedScale +
          wobble;
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

void DeflektorishScene::updateCameraPan(float dt) {
  if (cameraNode_ == nullptr) {
    return;
  }
  if (cameraPanDuration_ <= 0.0f) {
    cameraBaseWorld_ = cameraPanTargetWorld_;
    cameraBaseHeight_ = cameraPanTargetHeight_ > 0.0f
                            ? cameraPanTargetHeight_
                            : Deflektorish::kOrthographicHeight;
    cameraNode_->setOrthographicHeight(cameraBaseHeight_);
    return;
  }
  cameraPanTime_ = std::min(cameraPanTime_ + dt, cameraPanDuration_);
  const float t = std::clamp(cameraPanTime_ / cameraPanDuration_, 0.0f, 1.0f);
  const float travel = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
  cameraBaseWorld_ =
      glm::mix(cameraPanStartWorld_, cameraPanTargetWorld_, travel);
  cameraBaseHeight_ =
      glm::mix(cameraPanStartHeight_, cameraPanTargetHeight_, travel);
  cameraNode_->setOrthographicHeight(cameraBaseHeight_);
  if (cameraPanTime_ >= cameraPanDuration_) {
    cameraPanDuration_ = 0.0f;
    cameraBaseWorld_ = cameraPanTargetWorld_;
    cameraBaseHeight_ = cameraPanTargetHeight_;
    cameraNode_->setOrthographicHeight(cameraBaseHeight_);
  }
}

float DeflektorishScene::fittedOrthographicHeight(
    const RoomRuntime &room) const {
  const glm::vec2 sizePixels =
      glm::max((room.boundsMaxPixels - room.boundsMinPixels) +
                   glm::vec2{0.0f, kHudTopBandPixels},
               glm::vec2{640.0f, 360.0f});
  const float aspect = screenSize_.y > 0.0f && screenSize_.x > 0.0f
                           ? screenSize_.x / screenSize_.y
                           : 16.0f / 9.0f;
  constexpr float kPaddingPixels = 90.0f;
  const float heightFromY =
      (sizePixels.y + kPaddingPixels) * Deflektorish::kPixelToWorld;
  const float heightFromX =
      (sizePixels.x + kPaddingPixels) * Deflektorish::kPixelToWorld / aspect;
  unsigned int hash = 2166136261u;
  for (char c : room.name) {
    hash ^= static_cast<unsigned int>(static_cast<unsigned char>(c));
    hash *= 16777619u;
  }
  constexpr float kFramingScales[] = {0.74f, 1.12f, 0.86f, 1.26f,
                                      0.80f, 1.18f, 0.92f};
  const float framingScale =
      kFramingScales[hash % (sizeof(kFramingScales) / sizeof(kFramingScales[0]))];
  constexpr float kSafetyPaddingPixels = 36.0f;
  const float safeHeightFromY =
      (sizePixels.y + kSafetyPaddingPixels) * Deflektorish::kPixelToWorld;
  const float safeHeightFromX =
      (sizePixels.x + kSafetyPaddingPixels) * Deflektorish::kPixelToWorld /
      aspect;
  const float safeHeight = std::max(safeHeightFromY, safeHeightFromX);
  const float styledHeight = std::max(heightFromY, heightFromX) * framingScale;
  return std::clamp(std::max(styledHeight, safeHeight), 3.6f, 10.6f);
}

void DeflektorishScene::updateHudPositions() {
  const glm::vec2 center = cameraBaseWorld_;
  const float hudPixelToWorld =
      (cameraBaseHeight_ > 0.0f ? cameraBaseHeight_
                                : Deflektorish::kOrthographicHeight) /
      (Deflektorish::kScreenCenter.y * 2.0f);
  const auto placePlane = [&](DL::ShaderPlaneNode *node, glm::vec2 pixels,
                              glm::vec2 halfSizePixels, float z) {
    if (node == nullptr) {
      return;
    }
    const glm::vec2 world = center + pixels * hudPixelToWorld;
    node->setLocalPosition({world.x, world.y, z});
    node->setLocalScale({halfSizePixels.x * hudPixelToWorld,
                         halfSizePixels.y * hudPixelToWorld, 1.0f});
  };
  const auto placeText = [&](TextNode *node, glm::vec2 pixels, float z) {
    if (node == nullptr) {
      return;
    }
    const glm::vec2 world = center + pixels * hudPixelToWorld;
    node->setLocalPosition({world.x, world.y, z});
    node->setLocalScale({hudPixelToWorld, hudPixelToWorld, 1.0f});
  };

  const glm::vec2 virtualScreen = Deflektorish::kScreenCenter * 2.0f;
  const glm::vec2 topCenterSafe{virtualScreen.x * 0.5f,
                                virtualScreen.y - kHudSafePaddingPixels};

  placeText(scoreHud_, topCenterSafe - Deflektorish::kScreenCenter, 0.35f);
  const glm::vec2 energyHalfSize{64.0f, 6.0f};
  placePlane(energyBar_,
             topCenterSafe + glm::vec2{0.0f, -27.0f} -
                 Deflektorish::kScreenCenter,
             energyHalfSize, 0.20f);
  placePlane(completionOverlay_, {0.0f, 0.0f}, {520.0f, 350.0f}, 0.24f);
  placePlane(entryTransitionOverlay_, {0.0f, 0.0f}, {620.0f, 430.0f}, 0.44f);
  placeText(completionTitle_, {0.0f, 48.0f}, 0.34f);
  placeText(completionSubtitle_, {0.0f, 8.0f}, 0.34f);
  placeText(bonusHeading_, {0.0f, 48.0f}, 0.35f);
  placeText(bonusEnergy_, {0.0f, 14.0f}, 0.35f);
  placeText(bonusTime_, {0.0f, -12.0f}, 0.35f);
  placeText(bonusTotal_, {0.0f, -48.0f}, 0.35f);
  placeText(gameOverTitle_, {0.0f, 62.0f}, 0.36f);
  placeText(gameOverScoreText_, {0.0f, 10.0f}, 0.36f);
}

DeflektorishScene::BeamResult DeflektorishScene::solveBeam() {
  Deflektorish::BeamWorld world;
  world.sourcePosition = sourcePosition_;
  world.sourceAngle = sourceAngle_;

  world.reflektors.reserve(activeReflektorIndices_.size());
  for (std::size_t index : activeReflektorIndices_) {
    const Reflektor &reflektor = reflektors_[index];
    world.reflektors.push_back({reflektor.position, reflektor.angle});
  }

  world.targets.reserve(activeTargetIndices_.size());
  for (std::size_t index : activeTargetIndices_) {
    const Target &target = targets_[index];
    world.targets.push_back({target.position, target.alive});
  }

  world.blockers.reserve(activeBlockerIndices_.size());
  for (std::size_t index : activeBlockerIndices_) {
    const Blocker &blocker = blockers_[index];
    world.blockers.push_back({blocker.position, blocker.reflective});
  }

  world.portals.reserve(activePortalIndices_.size());
  for (std::size_t index : activePortalIndices_) {
    const Portal &portal = portals_[index];
    world.portals.push_back({portal.entryPosition, portal.exitPosition});
  }

  world.filters.reserve(activeFilterIndices_.size());
  for (std::size_t index : activeFilterIndices_) {
    const Filter &filter = filters_[index];
    world.filters.push_back({filter.position, filter.angle});
  }

  world.splitters.reserve(activeSplitterIndices_.size());
  for (std::size_t index : activeSplitterIndices_) {
    const Splitter &splitter = splitters_[index];
    world.splitters.push_back({splitter.position, splitter.angle});
  }

  return Deflektorish::solveBeamWorld(world, renderer_.beamSegmentCapacity());
}

DeflektorishScene::BeamResult DeflektorishScene::inactiveBeamResult() const {
  BeamResult result;
  result.activeReflektors.assign(activeReflektorIndices_.size(), false);
  result.reflektorEnergy.assign(activeReflektorIndices_.size(), 0.0f);
  result.reflektorHit.assign(activeReflektorIndices_.size(), glm::vec2(0.0f));
  result.reflektorHasHit.assign(activeReflektorIndices_.size(), false);
  result.activeBlockers.assign(activeBlockerIndices_.size(), false);
  result.blockerEnergy.assign(activeBlockerIndices_.size(), 0.0f);
  result.blockerHit.assign(activeBlockerIndices_.size(), glm::vec2(0.0f));
  result.blockerHasHit.assign(activeBlockerIndices_.size(), false);
  result.hitTargets.assign(activeTargetIndices_.size(), false);
  result.targetEnergy.assign(activeTargetIndices_.size(), 0.0f);
  result.activePortals.assign(activePortalIndices_.size(), false);
  result.portalEntryHit.assign(activePortalIndices_.size(), glm::vec2(0.0f));
  result.portalExitHit.assign(activePortalIndices_.size(), glm::vec2(0.0f));
  result.portalHasHit.assign(activePortalIndices_.size(), false);
  result.passingFilters.assign(activeFilterIndices_.size(), false);
  result.blockedFilters.assign(activeFilterIndices_.size(), false);
  result.filterHit.assign(activeFilterIndices_.size(), glm::vec2(0.0f));
  result.filterHasHit.assign(activeFilterIndices_.size(), false);
  result.activeSplitters.assign(activeSplitterIndices_.size(), false);
  result.splitterHit.assign(activeSplitterIndices_.size(), glm::vec2(0.0f));
  result.splitterHasHit.assign(activeSplitterIndices_.size(), false);
  return result;
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
    Deflektorish::setEnergyBarParams(
        energyBar_, ratio, beamEnergy_.danger,
        beamEnergy_.drainPerSecond /
            std::max(beamEnergyConfig_.maxDrainPerSecond, 0.001f),
        elapsed_);
  }
  const float visualDrainRange =
      std::max(beamEnergyConfig_.selfCrossDrainPerSecond * 3.0f, 0.001f);
  const float drainRatio = beamEnergy_.drainPerSecond / visualDrainRange;
  const float targetVisual =
      std::clamp(std::max(beamEnergy_.danger, drainRatio), 0.0f, 1.0f);
  beamDangerVisual_ = approach(beamDangerVisual_, targetVisual,
                               std::clamp(dt * 14.0f, 0.0f, 1.0f));
}

glm::vec2 DeflektorishScene::postBumpUvForWorld(glm::vec2 worldPosition) const {
  const float aspect = framebufferSize_.y > 0.0f
                           ? framebufferSize_.x / framebufferSize_.y
                           : 16.0f / 9.0f;
  const float orthoHeight =
      cameraNode_ != nullptr ? cameraNode_->orthographicHeight()
                             : Deflektorish::kOrthographicHeight;
  const float halfHeight = orthoHeight * 0.5f;
  const float halfWidth = halfHeight * aspect;
  const glm::vec2 cameraOffset =
      cameraNode_ != nullptr ? glm::vec2(cameraNode_->getLocalPosition())
                             : cameraBaseWorld_;
  const glm::vec2 relative = worldPosition - cameraOffset;
  return {0.5f + relative.x / (halfWidth * 2.0f),
          0.5f + relative.y / (halfHeight * 2.0f)};
}

glm::vec2 DeflektorishScene::screenToWorld(glm::vec2 screenPosition) const {
  if (screenSize_.x <= 0.0f || screenSize_.y <= 0.0f) {
    return {0.0f, 0.0f};
  }

  const float aspect = screenSize_.x / screenSize_.y;
  const float orthoHeight =
      cameraNode_ != nullptr ? cameraNode_->orthographicHeight()
                             : Deflektorish::kOrthographicHeight;
  const float halfHeight = orthoHeight * 0.5f;
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
    if (reflektor.automatic || !isCurrentRoom(reflektor.roomIndex)) {
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
  for (std::size_t activeIndex = 0; activeIndex < activeReflektorIndices_.size();
       ++activeIndex) {
    Reflektor &reflektor = reflektors_[activeReflektorIndices_[activeIndex]];
    if (result.reflektorHasHit[activeIndex]) {
      reflektor.hitPoint = result.reflektorHit[activeIndex];
    }
    renderer_.updateReflektor(
        reflektor.node, reflektor.glow, result.activeReflektors[activeIndex],
        reflektor.automatic,
        selectedReflektor_ ==
            static_cast<int>(activeReflektorIndices_[activeIndex]),
        result.reflektorEnergy[activeIndex],
        result.reflektorHasHit[activeIndex], reflektor.hitPoint, dt);
  }
}

void DeflektorishScene::updateBlockerVisuals(float dt,
                                             const BeamResult &result) {
  for (std::size_t activeIndex = 0; activeIndex < activeBlockerIndices_.size();
       ++activeIndex) {
    Blocker &blocker = blockers_[activeBlockerIndices_[activeIndex]];
    renderer_.updateBlocker(blocker.node, blocker.glow, blocker.energy,
                            blocker.hitPoint,
                            result.activeBlockers[activeIndex],
                            result.blockerEnergy[activeIndex],
                            result.blockerHasHit[activeIndex],
                            result.blockerHit[activeIndex], dt);
  }
}

void DeflektorishScene::updatePortalVisuals(float dt,
                                            const BeamResult &result) {
  for (std::size_t activeIndex = 0; activeIndex < activePortalIndices_.size();
       ++activeIndex) {
    Portal &portal = portals_[activePortalIndices_[activeIndex]];
    renderer_.updatePortal(
        portal.entryNode, portal.exitNode, portal.glow, portal.entryHitPoint,
        portal.exitHitPoint, portal.phase, result.activePortals[activeIndex],
        result.portalEntryHit[activeIndex], result.portalExitHit[activeIndex],
        result.portalHasHit[activeIndex], elapsed_, dt);
  }
}

void DeflektorishScene::updateFilterVisuals(float dt,
                                            const BeamResult &result) {
  for (std::size_t activeIndex = 0; activeIndex < activeFilterIndices_.size();
       ++activeIndex) {
    Filter &filter = filters_[activeFilterIndices_[activeIndex]];
    renderer_.updateFilter(filter.node, filter.passGlow, filter.blockGlow,
                           filter.hitPoint,
                           result.passingFilters[activeIndex],
                           result.blockedFilters[activeIndex],
                           result.filterHasHit[activeIndex],
                           result.filterHit[activeIndex], dt);
  }
}

void DeflektorishScene::updateSplitterVisuals(float dt,
                                              const BeamResult &result) {
  for (std::size_t activeIndex = 0; activeIndex < activeSplitterIndices_.size();
       ++activeIndex) {
    Splitter &splitter = splitters_[activeSplitterIndices_[activeIndex]];
    renderer_.updateSplitter(splitter.node, splitter.glow, splitter.hitPoint,
                             result.activeSplitters[activeIndex],
                             result.splitterHasHit[activeIndex],
                             result.splitterHit[activeIndex], dt);
  }
}

void DeflektorishScene::updateParallaxBackgrounds() {
  if (parallaxBackgrounds_.empty()) {
    return;
  }
  glm::vec2 cameraPosition = cameraBaseWorld_;
  if (cameraNode_ != nullptr) {
    const glm::vec3 localPosition = cameraNode_->getLocalPosition();
    cameraPosition = {localPosition.x, localPosition.y};
  }
  for (const auto &[node, roomIndex] : parallaxBackgrounds_) {
    Deflektorish::setParallaxBackgroundParams(
        node, elapsed_, cameraPosition, roomIndex);
  }
}

void DeflektorishScene::updateTargetState(float dt, const BeamResult &result) {
  for (std::size_t activeIndex = 0; activeIndex < activeTargetIndices_.size();
       ++activeIndex) {
    const std::size_t targetIndex = activeTargetIndices_[activeIndex];
    Target &target = targets_[targetIndex];
    if (result.hitTargets[activeIndex] && target.alive &&
        target.dying <= 0.0f) {
      target.dying = kTargetPrepopDuration;
      target.hitEnergy = result.targetEnergy[activeIndex];
      target.hitFlash = 1.0f;
      emitTargetFirstHit(targetIndex, target.position, target.hitEnergy);
    }
    target.hitFlash = std::max(target.hitFlash - dt * 6.5f, 0.0f);
    if (target.dying > 0.0f) {
      target.dying -= dt;
      if (target.dying <= 0.0f) {
        target.alive = false;
        emitTargetDestroyed(targetIndex, target.position, target.hitEnergy);
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
    postBumpCallback_(
        postBumpUvForWorld(Deflektorish::gameToWorld(event.position)),
        1.35f + event.energy * 0.28f);
  }
  if (soundCallback_) {
    soundCallback_(Deflektorish::Sound::TargetDestroyed, event.position,
                   event.energy);
  }
  spawnExplosion(event.position, event.energy);
  const glm::vec2 roomOffset = rooms_.empty()
                                   ? glm::vec2(0.0f)
                                   : rooms_[campaign_.currentLevelIndex()]
                                         .offsetPixels;
  startCameraShake(event.position - roomOffset,
                   kShakeStrength + event.energy * 0.85f,
                   0.34f + event.energy * 0.025f);
  if (event.index >= 0 &&
      static_cast<std::size_t>(event.index) < targets_.size()) {
    renderer_.hideNode(targets_[static_cast<std::size_t>(event.index)].node);
  }
  const int targetScore =
      kTargetClearScore +
      static_cast<int>(std::round(std::clamp(event.energy, 0.0f, 3.0f) *
                                  static_cast<float>(kTargetBeamEnergyScore)));
  campaign_.addScore(targetScore);
  scoreHudPulse_ = 1.0f;
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
  return !activeTargetIndices_.empty() &&
         std::all_of(activeTargetIndices_.begin(), activeTargetIndices_.end(),
                     [this](std::size_t index) { return !targets_[index].alive; });
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
  const glm::vec2 roomOffset = rooms_.empty()
                                   ? glm::vec2(0.0f)
                                   : rooms_[campaign_.currentLevelIndex()]
                                         .offsetPixels;
  return roomOffset + glm::vec2{x, y};
}

void DeflektorishScene::createCompletionOverlay() {
  completionOverlay_ = addShaderPlane(
      "level_complete_overlay",
      Deflektorish::shaderStyle(Deflektorish::ShaderStyle::CompletionOverlay),
      DL::BlendMode::Alpha,
      Deflektorish::kScreenCenter, {520.0f, 350.0f}, 21, 0.24f);
  if (completionOverlay_ != nullptr) {
    Deflektorish::clearShaderParams(completionOverlay_);
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

  auto gameOverTitle = std::make_unique<TextNode>(
      this, "GAME OVER", renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  gameOverTitle->setDebugName("game_over_title");
  gameOverTitle->setRenderLayer(23);
  gameOverTitle->setFontPixelHeight(46.0f);
  gameOverTitle->setTextAlignment(DL::TextAlignment::CENTER);
  gameOverTitle->setTextAnchor(DL::TextAnchor::CENTER);
  gameOverTitle->setTextColor({1.0f, 0.26f, 0.18f, 0.0f});
  gameOverTitle->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  gameOverTitle->setShadowOffset({2.0f, -2.0f});
  gameOverTitle->setLocalScale({Deflektorish::kPixelToWorld,
                                Deflektorish::kPixelToWorld, 1.0f});
  gameOverTitle_ = gameOverTitle.get();
  addChild(std::move(gameOverTitle));

  auto gameOverScore = std::make_unique<TextNode>(
      this, "SCORE  00000000", renderDevice_, renderResourceCache_,
      &cameraNode_->camera());
  gameOverScore->setDebugName("game_over_score");
  gameOverScore->setRenderLayer(23);
  gameOverScore->setFontPixelHeight(23.0f);
  gameOverScore->setTextAlignment(DL::TextAlignment::CENTER);
  gameOverScore->setTextAnchor(DL::TextAnchor::CENTER);
  gameOverScore->setTextColor({0.74f, 0.98f, 1.0f, 0.0f});
  gameOverScore->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
  gameOverScore->setShadowOffset({1.4f, -1.4f});
  gameOverScore->setLocalScale({Deflektorish::kPixelToWorld,
                                Deflektorish::kPixelToWorld, 1.0f});
  gameOverScoreText_ = gameOverScore.get();
  addChild(std::move(gameOverScore));

}

void DeflektorishScene::createEntryTransitionOverlay() {
  entryTransitionOverlay_ =
      addShaderPlane("entry_transition_overlay",
                     Deflektorish::shaderStyle(
                         Deflektorish::ShaderStyle::FadeTransition),
                     DL::BlendMode::Alpha, Deflektorish::kScreenCenter,
                     {620.0f, 430.0f}, 45, 0.44f);
  if (entryTransitionOverlay_ != nullptr) {
    entryTransitionOverlay_->config.color = {1.0f, 1.0f, 1.0f, 1.0f};
    Deflektorish::setFadeTransitionParams(entryTransitionOverlay_, elapsed_,
                                          1.0f, 0.0f, true);
  }
  entryTransition_.start(kEntryTransitionDuration);
}

void DeflektorishScene::updateEntryTransition(float dt) {
  if (entryTransitionOverlay_ == nullptr) {
    return;
  }
  entryTransition_.update(dt);
  const float alpha = entryTransition_.fadeOutAlpha();
  entryTransitionOverlay_->config.color = {1.0f, 1.0f, 1.0f, alpha};
  Deflektorish::setFadeTransitionParams(entryTransitionOverlay_, elapsed_,
                                        alpha, entryTransition_.progress(),
                                        true);
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
  lastGameOverScoreText_.clear();
  updateBonusText();
  updateGameOverText();
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
        postBumpCallback_({0.5f, 0.5f}, 1.85f);
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
      const glm::vec2 roomOffset = rooms_.empty()
                                       ? glm::vec2(0.0f)
                                       : rooms_[campaign_.currentLevelIndex()]
                                             .offsetPixels;
      startCameraShake(position - roomOffset,
                       kShakeStrength * 0.34f + energy * 0.22f, 0.18f);
      ++victoryBlastIndex_;
      victoryBlastTimer_ += kVictoryBlastInterval;
    }
    if (victoryBlastIndex_ >= kVictoryBlastCount && !anyExplosionActive()) {
      completionPhase_ = CompletionPhase::FadeOut;
      completionFadeTime_ = 0.0f;
      victoryCelebrationComplete_ = true;
      startBonusTally();
    }
  } else if (completionPhase_ == CompletionPhase::FadeOut) {
    completionFadeTime_ += dt;
    updateBonusTally(dt);
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
  } else if (completionPhase_ == CompletionPhase::BonusPending ||
             bonusTallyPhase_ != BonusTallyPhase::Hidden) {
    fade = 0.0f;
  }
  const bool bonusVisible = bonusTallyPhase_ != BonusTallyPhase::Hidden &&
                            bonusTallyPhase_ != BonusTallyPhase::Done;
  const float bonusBackdrop =
      bonusVisible ? 0.34f : 1.0f;
  const float bandA = bonusVisible ? 0.14f : 0.26f;
  const float bandB = bonusVisible ? -0.14f : -0.10f;
  const float overlayAlpha =
      easedIntro * (0.58f + pulse * 0.08f) *
      std::max(fade, bonusBackdrop);

  Deflektorish::setCompletionOverlayParams(completionOverlay_, victoryTime_,
                                           overlayAlpha, blastProgress,
                                           easedIntro, fade, bonusBackdrop,
                                           bandA, bandB);
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
  totalBonus_ = campaign_.score() + energyBonus_ + timeBonus_;
  displayedEnergyBonus_ = 0;
  displayedTimeBonus_ = 0;
  displayedTotalBonus_ = campaign_.score();
  bonusPhaseTime_ = 0.0f;
  bonusFadeTime_ = 0.0f;
  bonusScoreTickTimer_ = 0.0f;
  bonusTallyPhase_ = BonusTallyPhase::Energy;
  updateBonusText();
  if (rooms_.size() > 1) {
    activateRoom(campaign_.currentLevelIndex() + 1, true, true);
  }
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
      completionPhase_ = CompletionPhase::Playing;
      victoryCelebrationStarted_ = false;
      victoryCelebrationComplete_ = false;
      Deflektorish::setCompletionOverlayParams(
          completionOverlay_, victoryTime_, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
      if (completionTitle_ != nullptr) {
        completionTitle_->setTextColor({0.64f, 0.96f, 1.0f, 0.0f});
        completionTitle_->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
      }
      if (completionSubtitle_ != nullptr) {
        completionSubtitle_->setTextColor({1.0f, 0.72f, 0.28f, 0.0f});
        completionSubtitle_->setShadowColor({0.0f, 0.02f, 0.05f, 0.0f});
      }
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
        bonusPhaseTime_ >= kBonusLineDelay + 0.12f) {
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
        bonusPhaseTime_ >= kBonusLineDelay + 0.12f) {
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
      campaign_.setScore(totalBonus_);
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

void DeflektorishScene::startGameOver() {
  if (completionPhase_ == CompletionPhase::GameOver) {
    return;
  }

  resetBonusTally();
  completionPhase_ = CompletionPhase::GameOver;
  victoryCelebrationStarted_ = false;
  victoryCelebrationComplete_ = false;
  gameOverTime_ = 0.0f;
  gameOverRankFlash_ = 1.0f;
  previousGameOverFireDown_ = true;
  gameOverCallbackDispatched_ = false;
  gameOverScore_ = campaign_.score();
  renderer_.hideNode(selection_);
  if (soundCallback_) {
    soundCallback_(Deflektorish::Sound::GameOver, Deflektorish::kScreenCenter,
                   0.0f);
  }
  if (completionOverlay_ != nullptr) {
    Deflektorish::setCompletionOverlayParams(completionOverlay_, 0.0f, 0.70f,
                                             1.0f, 1.0f, 1.0f, 0.72f, 0.34f,
                                             -0.12f);
  }
  startCameraShake(Deflektorish::kScreenCenter, kShakeStrength * 0.82f, 0.34f);
  updateGameOverText();
}

void DeflektorishScene::updateGameOver(float dt, bool fireDown) {
  if (completionPhase_ != CompletionPhase::GameOver) {
    previousGameOverFireDown_ = fireDown;
    updateGameOverText();
    return;
  }

  gameOverTime_ += dt;
  gameOverRankFlash_ = std::max(gameOverRankFlash_ - dt * 2.8f, 0.0f);
  const bool canSkip = gameOverTime_ > kGameOverSkipDelay;
  const bool shouldLeave =
      gameOverTime_ >= kGameOverReturnDelay ||
      (canSkip && fireDown && !previousGameOverFireDown_);
  if (shouldLeave && !gameOverCallbackDispatched_) {
    gameOverCallbackDispatched_ = true;
    if (gameOverCallback_) {
      gameOverCallback_(gameOverScore_);
    }
    previousGameOverFireDown_ = fireDown;
    return;
  }
  previousGameOverFireDown_ = fireDown;
  if (completionOverlay_ != nullptr) {
    const float pulse = 0.5f + 0.5f * std::sin(elapsed_ * 5.0f);
    Deflektorish::setCompletionOverlayParams(
        completionOverlay_, gameOverTime_, 0.58f + pulse * 0.08f, 1.0f, 1.0f,
        1.0f, 0.78f, 0.34f, -0.12f);
  }
  updateGameOverText();
}

void DeflektorishScene::updateGameOverText() {
  const bool visible = completionPhase_ == CompletionPhase::GameOver;
  const float intro =
      visible ? std::clamp(gameOverTime_ / 0.34f, 0.0f, 1.0f) : 0.0f;
  const float alpha = intro * intro * (3.0f - 2.0f * intro);
  const float pulse = 0.5f + 0.5f * std::sin(elapsed_ * 8.0f);
  const float promptPulse = 0.5f + 0.5f * std::sin(elapsed_ * 7.0f);

  if (gameOverTitle_ != nullptr) {
    gameOverTitle_->setTextColor(
        {1.0f, 0.20f + pulse * 0.12f, 0.16f, alpha});
    gameOverTitle_->setShadowColor({0.0f, 0.02f, 0.05f, alpha * 0.76f});
  }

  const std::string scoreText = "SCORE  " + scoreDigits(gameOverScore_);
  if (gameOverScoreText_ != nullptr) {
    if (lastGameOverScoreText_ != scoreText) {
      gameOverScoreText_->setText(scoreText);
      lastGameOverScoreText_ = scoreText;
    }
    gameOverScoreText_->setTextColor({0.70f + pulse * 0.10f, 0.96f, 1.0f,
                                      alpha});
    gameOverScoreText_->setShadowColor({0.0f, 0.02f, 0.05f, alpha * 0.70f});
  }

}

void DeflektorishScene::updateScoreHud(float dt) {
  if (scoreHud_ == nullptr) {
    return;
  }

  int targetScore = campaign_.score();
  if (bonusTallyPhase_ == BonusTallyPhase::Total ||
      bonusTallyPhase_ == BonusTallyPhase::Hold ||
      bonusTallyPhase_ == BonusTallyPhase::FadeOut ||
      bonusTallyPhase_ == BonusTallyPhase::Done) {
    targetScore = std::max(targetScore, displayedTotalBonus_);
  }

  scoreHudRollTime_ += dt;
  const int previous = displayedHudScore_;
  if (displayedHudScore_ < targetScore) {
    const int remaining = targetScore - displayedHudScore_;
    const int smoothStep =
        std::max(1, static_cast<int>(std::ceil(static_cast<float>(remaining) *
                                               std::min(dt * 9.0f, 0.42f))));
    const int arcadeStep =
        remaining > 250 ? std::max(25, (smoothStep / 25) * 25) : smoothStep;
    displayedHudScore_ =
        std::min(displayedHudScore_ + arcadeStep, targetScore);
  } else if (displayedHudScore_ > targetScore) {
    displayedHudScore_ = targetScore;
  }

  if (displayedHudScore_ != previous) {
    scoreHudPulse_ = 1.0f;
  } else {
    scoreHudPulse_ = std::max(scoreHudPulse_ - dt * 5.5f, 0.0f);
  }

  const std::string text = scoreDigits(displayedHudScore_);
  if (lastScoreHudText_ != text) {
    scoreHud_->setText(text);
    lastScoreHudText_ = text;
  }

  const float shimmer = 0.5f + 0.5f * std::sin(elapsed_ * 18.0f);
  const float rollFlicker =
      displayedHudScore_ != targetScore
          ? 0.5f + 0.5f * std::sin(scoreHudRollTime_ * 56.0f)
          : 0.0f;
  const float hot = std::clamp(scoreHudPulse_ * 0.86f + rollFlicker * 0.22f,
                               0.0f, 1.0f);
  const glm::vec3 cool{0.58f + shimmer * 0.08f, 0.92f, 1.0f};
  const glm::vec3 warm{1.0f, 0.78f + shimmer * 0.10f, 0.24f};
  const glm::vec3 color = glm::mix(cool, warm, hot);
  const float alpha = 0.88f + shimmer * 0.04f + hot * 0.08f;
  scoreHud_->setTextColor({color.r, color.g, color.b, alpha});
  scoreHud_->setShadowColor({0.0f, 0.02f, 0.05f, 0.76f + hot * 0.18f});
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

  const float desiredTicks = total ? 10.0f : 8.0f;
  const float rawStep = static_cast<float>(target) / desiredTicks;
  const int quantum = 100;
  int step = static_cast<int>(std::round(rawStep / static_cast<float>(quantum))) *
             quantum;
  step = std::clamp(step, quantum, total ? 1800 : 1000);
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
    cameraNode_->setLocalPosition({cameraBaseWorld_.x, cameraBaseWorld_.y,
                                   10.5f});
    cameraNode_->lookAtWorld({cameraBaseWorld_.x, cameraBaseWorld_.y, 0.0f});
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
  const glm::vec2 cameraPosition = cameraBaseWorld_ + offset;
  cameraNode_->setLocalPosition({cameraPosition.x, cameraPosition.y, 10.5f});
  cameraNode_->lookAtWorld({cameraPosition.x, cameraPosition.y, 0.0f});
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
    if (!reflektors_[index].automatic &&
        isCurrentRoom(reflektors_[index].roomIndex)) {
      return index;
    }
  }
  return -1;
}
