#include "deflektorishintroscene.h"

#include "game/deflektorish/deflektorishconfig.h"
#include "game/deflektorish/deflektorishshaderparams.h"
#include "game/deflektorish/deflektorishshaderstyle.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace {
constexpr float kPixelToWorld = Deflektorish::kPixelToWorld;
constexpr float kAttractPageSeconds = 3.4f;
constexpr float kStartFadeToBlackDuration = 0.24f;

struct ScoreEntry {
  const char *name;
  int score;
};

constexpr ScoreEntry kHighScores[] = {
    {"ACE", 98500}, {"LUX", 84200}, {"RAY", 73150},
    {"KID", 60900}, {"CPU", 42750},
};

std::string scoreText(int rank, const ScoreEntry &entry) {
  std::string value = std::to_string(std::max(entry.score, 0));
  while (value.size() < 6) {
    value.insert(value.begin(), '0');
  }
  return std::to_string(rank) + "  " + entry.name + "  " + value;
}

float angleOf(glm::vec2 direction) {
  return std::atan2(direction.y, direction.x);
}

float angleDelta(float a, float b) {
  return std::abs(std::atan2(std::sin(a - b), std::cos(a - b)));
}

glm::vec2 rotateAround(glm::vec2 point, glm::vec2 center, float radians) {
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  const glm::vec2 local = point - center;
  return center + glm::vec2{local.x * c - local.y * s,
                            local.x * s + local.y * c};
}

glm::vec2 normalizedOr(glm::vec2 value, glm::vec2 fallback) {
  const float length = glm::length(value);
  if (length <= 0.001f) {
    return fallback;
  }
  return value / length;
}

float mirrorAngleFor(glm::vec2 previous, glm::vec2 current, glm::vec2 next,
                     float fallbackAngle) {
  const glm::vec2 incoming =
      normalizedOr(current - previous, {std::cos(fallbackAngle),
                                        std::sin(fallbackAngle)});
  const glm::vec2 outgoing = normalizedOr(next - current, incoming);
  const glm::vec2 mirrorDirection = incoming + outgoing;
  if (glm::length(mirrorDirection) <= 0.001f) {
    return fallbackAngle;
  }
  return angleOf(mirrorDirection);
}

glm::vec2 sineBlobPoint(glm::vec2 base, float elapsed, float speed,
                        float phase, float index, glm::vec2 amplitude) {
  const float t = elapsed * speed;
  return base +
         glm::vec2{
             std::sin(t + phase) * amplitude.x +
                 std::sin(t * 0.41f + phase * 1.73f + index * 0.91f) *
                     amplitude.x * 0.18f +
                 std::cos(t * 0.23f + phase * 0.47f + index * 1.31f) *
                     amplitude.x * 0.12f,
             std::cos(t * 0.83f + phase * 1.19f) * amplitude.y +
                 std::sin(t * 0.36f + phase * 0.67f + index * 1.57f) *
                     amplitude.y * 0.20f +
                 std::cos(t * 0.19f + phase * 2.11f + index * 0.37f) *
                     amplitude.y * 0.10f};
}

} // namespace

DeflektorishIntroScene::DeflektorishIntroScene(
    DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *renderResourceCache,
    std::function<void()> startCallback)
    : renderDevice_(renderDevice), renderResourceCache_(renderResourceCache),
      startCallback_(std::move(startCallback)) {}

void DeflektorishIntroScene::init() {
  setDebugName("deflektorish_intro_scene");
  camera_.mProjection = DL::CameraProjection::Orthographic;
  camera_.mOrthographicHeight = Deflektorish::kOrthographicHeight;
  camera_.lookAt({0.0f, 0.0f, 0.0f});
  backgroundCamera_.mProjection = DL::CameraProjection::Orthographic;
  backgroundCamera_.mOrthographicHeight = Deflektorish::kOrthographicHeight;
  backgroundCamera_.lookAt({0.0f, 0.0f, 0.0f});

  auto *backplate =
      addPlane("intro_backplate", 0, DL::BlendMode::Opaque, {480.0f, 320.0f},
               {920.0f, 520.0f}, -20, -0.08f);
  backplate->config.color = {0.002f, 0.003f, 0.008f, 1.0f};

  createLiveShowcase();
  renderer_.createBeamSegments(this, &backgroundCamera_, renderDevice_,
                               renderResourceCache_, 18);
  addText("DEFLEKTORISH", {480.0f, 524.0f}, 52.0f,
          {0.74f, 0.98f, 1.0f, 0.96f}, 12);
  addHighScores();
  pressFire_ = addText("PRESS FIRE", {480.0f, 146.0f}, 24.0f,
                       {1.0f, 0.82f, 0.32f, 0.95f}, 12);
  addCredits();
  transitionOverlay_ =
      addPlane("intro_start_transition",
               Deflektorish::shaderStyle(
                   Deflektorish::ShaderStyle::FadeTransition),
               DL::BlendMode::Alpha, {480.0f, 320.0f}, {620.0f, 430.0f},
               40, 0.32f);
  if (transitionOverlay_ != nullptr) {
    transitionOverlay_->config.color = {1.0f, 1.0f, 1.0f, 0.0f};
    Deflektorish::setFadeTransitionParams(transitionOverlay_, elapsed_, 0.0f,
                                          0.0f, false);
  }

  SceneNode::init();
  for (auto &child : children) {
    child->init();
  }
  updateLayout();
}

void DeflektorishIntroScene::update(const DL::FrameContext &ctx) {
  elapsed_ += ctx.delta_time;
  const bool fireDown = ctx.input.isActionDown(DL::Action::Fire);
  if (fireDown && !previousFireDown_) {
    requestStart();
  }
  previousFireDown_ = fireDown;

  updateBackgroundCamera();
  updateLiveShowcase(ctx.delta_time);
  updateStartTransition(ctx.delta_time);

  if (pressFire_ != nullptr) {
    const float blink = 0.58f + 0.42f * std::sin(elapsed_ * 7.0f);
    const float fade = startRequested_ ? 1.0f - startTransition_.progress()
                                       : 1.0f;
    pressFire_->setTextColor({1.0f, 0.62f + blink * 0.28f, 0.20f,
                              (0.48f + blink * 0.48f) * fade});
  }
  updateAttractPage();

  SceneNode::update(ctx);
}

void DeflektorishIntroScene::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void DeflektorishIntroScene::onClick(double, double) { requestStart(); }

void DeflektorishIntroScene::onKey(int) {}

void DeflektorishIntroScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  screenSize_ = size;
  camera_.mScreenSize = size;
  backgroundCamera_.mScreenSize = size;
  updateLayout();
}

void DeflektorishIntroScene::onFramebufferSizeChanged(glm::vec2 size) {
  framebufferSize_ = size;
}

TextNode *DeflektorishIntroScene::addText(std::string text,
                                          glm::vec2 positionPixels,
                                          float pixelHeight, glm::vec4 color,
                                          int renderLayer) {
  auto node = std::make_unique<TextNode>(this, std::move(text), renderDevice_,
                                         renderResourceCache_, &camera_);
  node->setRenderLayer(renderLayer);
  node->setFontPixelHeight(pixelHeight);
  node->setTextAlignment(DL::TextAlignment::CENTER);
  node->setTextAnchor(DL::TextAnchor::CENTER);
  node->setTextColor(color);
  node->setShadowColor({0.0f, 0.03f, 0.05f, 0.80f});
  node->setShadowOffset({1.4f, -1.4f});
  node->setLocalPosition(toWorld(positionPixels, 0.15f));
  node->setLocalScale({kPixelToWorld, kPixelToWorld, 1.0f});
  TextNode *raw = node.get();
  addChild(std::move(node));
  return raw;
}

TextNode *DeflektorishIntroScene::addLeftText(std::string text,
                                              glm::vec2 positionPixels,
                                              float pixelHeight,
                                              glm::vec4 color,
                                              int renderLayer) {
  TextNode *node = addText(std::move(text), positionPixels, pixelHeight, color,
                           renderLayer);
  node->setTextAlignment(DL::TextAlignment::LEFT);
  node->setTextAnchor(DL::TextAnchor::CENTER_LEFT);
  return node;
}

DL::ShaderPlaneNode *DeflektorishIntroScene::addPlane(
    std::string name, int style, DL::BlendMode blendMode,
    glm::vec2 positionPixels, glm::vec2 halfSizePixels, int renderLayer,
    float z, float rotationRadians) {
  return addPlaneForCamera(std::move(name), style, blendMode, positionPixels,
                           halfSizePixels, renderLayer, &camera_, z,
                           rotationRadians);
}

DL::ShaderPlaneNode *DeflektorishIntroScene::addPlaneForCamera(
    std::string name, int style, DL::BlendMode blendMode,
    glm::vec2 positionPixels, glm::vec2 halfSizePixels, int renderLayer,
    DL::Camera *camera, float z, float rotationRadians) {
  DL::ShaderPlaneNode::Config config;
  config.fragmentShader = "Shaders/deflektorish.frag";
  config.blendMode = blendMode;
  config.depthTest = false;
  config.proceduralStyle = style;
  config.color = {1.0f, 1.0f, 1.0f, 1.0f};
  config.params0 = {0.0f, 0.0f, 0.0f, 0.0f};
  config.params1 = {0.0f, 0.0f, 0.0f, 0.0f};
  auto node = std::make_unique<DL::ShaderPlaneNode>(
      config, this, camera, renderDevice_, renderResourceCache_);
  node->setDebugName(std::move(name));
  node->setRenderLayer(renderLayer);
  node->setLocalPosition(toWorld(positionPixels, z));
  node->setLocalScale(scaleToWorld(halfSizePixels));
  node->setLocalRotation(glm::quat(glm::vec3(0.0f, 0.0f, rotationRadians)));
  DL::ShaderPlaneNode *raw = node.get();
  addChild(std::move(node));
  return raw;
}

void DeflektorishIntroScene::addHighScores() {
  highScoreTexts_.push_back(addText("HIGH SCORES", {480.0f, 398.0f}, 24.0f,
                                    {1.0f, 0.74f, 0.28f, 0.94f}, 12));
  float y = 356.0f;
  for (int i = 0; i < static_cast<int>(std::size(kHighScores)); ++i) {
    const glm::vec4 color =
        i == 0 ? glm::vec4{0.78f, 1.0f, 0.98f, 0.96f}
               : glm::vec4{0.76f, 0.86f, 0.95f, 0.86f};
    highScoreTexts_.push_back(
        addText(scoreText(i + 1, kHighScores[i]), {480.0f, y}, 22.0f, color,
                12));
    y -= 30.0f;
  }
}

void DeflektorishIntroScene::addCredits() {
  creditTexts_.push_back(addText("CREDITS", {480.0f, 398.0f}, 24.0f,
                                 {1.0f, 0.74f, 0.28f, 0.94f}, 12));
  const glm::vec4 color{0.66f, 0.88f, 1.0f, 0.82f};
  creditTexts_.push_back(
      addText("CODE  DEURELL", {480.0f, 342.0f}, 20.0f, color, 12));
  creditTexts_.push_back(
      addText("GFX   DEURELL", {480.0f, 302.0f}, 20.0f, color, 12));
  creditTexts_.push_back(
      addText("SFX   KENNEY", {480.0f, 262.0f}, 20.0f, color, 12));
}

void DeflektorishIntroScene::createLiveShowcase() {
  const auto addSource = [&](glm::vec2 position, float angle, float phase) {
    IntroSource source;
    source.basePosition = position;
    source.position = position;
    source.angle = angle;
    source.baseAngle = angle;
    source.phase = phase;
    source.node =
        addPlaneForCamera("intro_demo_source",
                          Deflektorish::shaderStyle(
                              Deflektorish::ShaderStyle::Source),
                          DL::BlendMode::Additive, position, {24.0f, 24.0f},
                          10, &backgroundCamera_, 0.02f, angle);
    source.node->config.color = {0.55f, 0.76f, 1.0f, 0.26f};
    demoSources_.push_back(source);
  };

  const auto addReflektor = [&](glm::vec2 position, float angle, float speed,
                                float phase) {
    IntroReflektor reflektor;
    reflektor.basePosition = position;
    reflektor.position = position;
    reflektor.angle = angle;
    reflektor.baseAngle = angle;
    reflektor.previousAngle = angle;
    reflektor.speed = speed;
    reflektor.phase = phase;
    reflektor.node =
        addPlaneForCamera("intro_demo_reflector",
                          Deflektorish::shaderStyle(
                              Deflektorish::ShaderStyle::AutoReflector),
                          DL::BlendMode::Alpha, position, {28.0f, 6.0f}, 11,
                          &backgroundCamera_, 0.04f, angle);
    reflektor.node->config.color = {0.30f, 0.48f, 0.66f, 1.0f};
    Deflektorish::setReflectorOcclusionMode(reflektor.node, true);
    demoReflektors_.push_back(reflektor);
  };

  addSource({140.0f, 390.0f}, 0.0f, 0.0f);
  addSource({825.0f, 270.0f}, 0.0f, 2.4f);

  addReflektor({275.0f, 505.0f}, 0.0f, 0.58f, 1.0f);
  addReflektor({760.0f, 405.0f}, 0.0f, 0.45f, 1.8f);
  addReflektor({220.0f, 180.0f}, 0.0f, 0.64f, 2.6f);
  addReflektor({700.0f, 140.0f}, 0.0f, 0.52f, 3.4f);
}

void DeflektorishIntroScene::updateBackgroundCamera() {
  const glm::vec2 panPixels{
      std::sin(elapsed_ * 0.16f) * 58.0f +
          std::sin(elapsed_ * 0.41f) * 18.0f,
      std::cos(elapsed_ * 0.18f) * 38.0f +
          std::sin(elapsed_ * 0.29f) * 12.0f};
  const float zoom =
      5.35f + std::sin(elapsed_ * 0.13f) * 0.22f +
      std::sin(elapsed_ * 0.057f) * 0.12f;
  backgroundCamera_.mOrthographicHeight = zoom;
  backgroundCamera_.setPosition({panPixels.x * kPixelToWorld,
                                 panPixels.y * kPixelToWorld, 10.0f});
  backgroundCamera_.lookAt({panPixels.x * kPixelToWorld,
                            panPixels.y * kPixelToWorld, 0.0f});
}

void DeflektorishIntroScene::updateLiveShowcase(float dt) {
  const glm::vec2 globalDrift{
      std::sin(elapsed_ * 0.09f) * 14.0f +
          std::sin(elapsed_ * 0.047f + 1.7f) * 10.0f,
      std::cos(elapsed_ * 0.08f + 0.6f) * 10.0f};
  const glm::vec2 rotationCenter{480.0f, 320.0f};
  const float zSwirl =
      std::sin(elapsed_ * 0.125f) * 0.245f +
      std::sin(elapsed_ * 0.049f + 1.2f) * 0.135f;

  for (std::size_t i = 0; i < demoReflektors_.size(); ++i) {
    IntroReflektor &reflektor = demoReflektors_[i];
    const float index = static_cast<float>(i);
    const glm::vec2 amplitude{
        72.0f + std::sin(index * 1.31f) * 18.0f,
        48.0f + std::cos(index * 1.73f) * 13.0f};
    const float independentSpeed =
        reflektor.speed * (1.36f + std::sin(index * 2.03f) * 0.32f);
    reflektor.position =
        rotateAround(sineBlobPoint(reflektor.basePosition, elapsed_,
                                   reflektor.speed,
                                   reflektor.phase + index * 0.37f, index,
                                   amplitude) +
                         sineBlobPoint({0.0f, 0.0f}, elapsed_,
                                       independentSpeed,
                                       reflektor.phase * 1.71f + index,
                                       index + 4.0f, {20.0f, 16.0f}) +
                         globalDrift,
                     rotationCenter, zSwirl);
  }

  IntroSource *source = demoSources_.empty() ? nullptr : &demoSources_.front();
  IntroSource *sink = demoSources_.size() > 1 ? &demoSources_[1] : nullptr;
  if (source != nullptr) {
    source->position =
        rotateAround(sineBlobPoint(source->basePosition, elapsed_, 0.33f,
                                   source->phase, -1.0f, {30.0f, 24.0f}) +
                         globalDrift * 0.65f,
                     rotationCenter, zSwirl);
  }
  if (sink != nullptr) {
    sink->position =
        rotateAround(sineBlobPoint(sink->basePosition, elapsed_, 0.30f,
                                   sink->phase, 8.0f, {32.0f, 26.0f}) +
                         globalDrift * 0.72f,
                     rotationCenter, zSwirl);
  }
  if (source != nullptr) {
    if (!demoReflektors_.empty()) {
      source->angle = angleOf(demoReflektors_.front().position -
                              source->position);
    } else {
      source->angle = source->baseAngle;
    }
  }
  if (sink != nullptr) {
    const glm::vec2 incoming =
        !demoReflektors_.empty()
            ? sink->position - demoReflektors_.back().position
            : glm::vec2{1.0f, 0.0f};
    sink->angle = angleOf(incoming);
  }

  const glm::vec2 exitPoint =
      sink != nullptr
          ? sink->position
          : (source != nullptr
                 ? source->position
                 : glm::vec2{865.0f, 150.0f});
  for (std::size_t i = 0; i < demoReflektors_.size(); ++i) {
    IntroReflektor &reflektor = demoReflektors_[i];
    const glm::vec2 previous =
        i == 0
            ? (source != nullptr ? source->position : reflektor.basePosition)
            : demoReflektors_[i - 1].position;
    const glm::vec2 next =
        i + 1 < demoReflektors_.size() ? demoReflektors_[i + 1].position
                                       : exitPoint;
    const float solvedAngle =
        mirrorAngleFor(previous, reflektor.position, next, reflektor.baseAngle);
    const float aimAmount =
        std::clamp(angleDelta(solvedAngle, reflektor.previousAngle) * 6.0f,
                   0.0f, 1.0f);
    reflektor.aimFlash +=
        (aimAmount - reflektor.aimFlash) * std::clamp(dt * 9.0f, 0.0f, 1.0f);
    reflektor.previousAngle = solvedAngle;
    reflektor.angle = solvedAngle;
    if (reflektor.node != nullptr) {
      reflektor.node->setLocalPosition(toWorld(reflektor.position, 0.04f));
      reflektor.node->setLocalRotation(
          glm::quat(glm::vec3(0.0f, 0.0f, reflektor.angle)));
    }
  }

  if (source != nullptr && source->node != nullptr) {
    source->node->setLocalPosition(toWorld(source->position, 0.02f));
    source->node->setLocalRotation(
        glm::quat(glm::vec3(0.0f, 0.0f, source->angle)));
  }

  Deflektorish::BeamWorld world;
  world.sourcePosition = source != nullptr ? source->position : glm::vec2{0.0f};
  world.sourceAngle = source != nullptr ? source->angle : 0.0f;
  world.reflektors.reserve(demoReflektors_.size());
  for (const IntroReflektor &reflektor : demoReflektors_) {
    world.reflektors.push_back({reflektor.position, reflektor.angle});
  }
  if (sink != nullptr) {
    world.targets.push_back({sink->position, true});
  }
  Deflektorish::BeamSolveResult result =
      Deflektorish::solveBeamWorld(world, renderer_.beamSegmentCapacity());
  if (sink != nullptr && !result.segments.empty() &&
      !result.hitTargets.empty() && result.hitTargets.front() &&
      result.segments.back().endType == Deflektorish::BeamSegmentEnd::Target) {
    result.segments.back().end = sink->position;
  }
  renderer_.updateBeamSegments(result);
  if (source != nullptr) {
    renderer_.updateSource(source->node, elapsed_ + source->phase, 0.8f, 1.0f);
  }
  if (sink != nullptr && sink->node != nullptr) {
    const bool active = !result.hitTargets.empty() && result.hitTargets.front();
    renderer_.updateSource(sink->node, elapsed_ + sink->phase,
                           active ? 1.0f : 0.45f, active ? 1.0f : 0.35f);
    sink->node->setLocalPosition(toWorld(sink->position, 0.02f));
    sink->node->setLocalRotation(
        glm::quat(glm::vec3(0.0f, 0.0f, sink->angle)));
  }

  for (std::size_t i = 0; i < demoReflektors_.size(); ++i) {
    IntroReflektor &reflektor = demoReflektors_[i];
    const bool active =
        i < result.activeReflektors.size() && result.activeReflektors[i];
    const float energy =
        i < result.reflektorEnergy.size() ? result.reflektorEnergy[i] : 0.0f;
    renderer_.updateReflektor(reflektor.node, reflektor.glow, active, true,
                              false, energy, dt);
  }

  const float breathe = 0.78f + 0.22f * std::sin(elapsed_ * 0.85f);
  for (IntroSource &source : demoSources_) {
    if (source.node != nullptr) {
      source.node->config.color = {0.48f, 0.70f, 1.0f,
                                   0.16f + breathe * 0.10f};
    }
  }
  for (IntroReflektor &reflektor : demoReflektors_) {
    if (reflektor.node != nullptr) {
      const float active = std::clamp(reflektor.glow, 0.0f, 1.0f);
      const float aim = std::clamp(reflektor.aimFlash, 0.0f, 1.0f);
      reflektor.node->config.color =
          {0.26f + active * 0.08f + aim * 0.10f,
           0.42f + active * 0.12f + aim * 0.10f,
           0.62f + active * 0.16f + aim * 0.18f, 1.0f};
    }
  }
}

void DeflektorishIntroScene::updateAttractPage() {
  const float pageTime = std::fmod(elapsed_, kAttractPageSeconds * 2.0f);
  const bool showCredits = pageTime >= kAttractPageSeconds;
  const float localTime =
      showCredits ? pageTime - kAttractPageSeconds : pageTime;
  const float fadeIn = std::clamp(localTime / 0.28f, 0.0f, 1.0f);
  const float fadeOut =
      std::clamp((kAttractPageSeconds - localTime) / 0.28f, 0.0f, 1.0f);
  const float activeAlpha = std::min(fadeIn, fadeOut);
  const float inactiveAlpha = 0.0f;

  const auto applyAlpha = [](std::vector<TextNode *> &nodes, float alpha) {
    for (TextNode *node : nodes) {
      if (node == nullptr) {
        continue;
      }
      glm::vec4 color = node->textColor();
      color.a = alpha;
      node->setTextColor(color);
      node->setShadowColor({0.0f, 0.03f, 0.05f, alpha * 0.84f});
    }
  };
  applyAlpha(highScoreTexts_, showCredits ? inactiveAlpha : activeAlpha);
  applyAlpha(creditTexts_, showCredits ? activeAlpha : inactiveAlpha);
}

void DeflektorishIntroScene::updateStartTransition(float dt) {
  if (!startRequested_) {
    if (transitionOverlay_ != nullptr) {
      transitionOverlay_->config.color = {1.0f, 1.0f, 1.0f, 0.0f};
      Deflektorish::setFadeTransitionParams(transitionOverlay_, elapsed_,
                                            0.0f, 0.0f, false);
    }
    return;
  }

  startTransition_.update(dt);
  const float alpha = startTransition_.fadeInAlpha();
  if (transitionOverlay_ != nullptr) {
    transitionOverlay_->config.color = {1.0f, 1.0f, 1.0f, 1.0f};
    Deflektorish::setFadeTransitionParams(
        transitionOverlay_, elapsed_, alpha, startTransition_.progress(), false);
  }

  if (!startCallbackDispatched_ && startTransition_.complete()) {
    startCallbackDispatched_ = true;
    if (startCallback_) {
      startCallback_();
    }
  }
}

void DeflektorishIntroScene::requestStart() {
  if (startRequested_) {
    return;
  }
  startRequested_ = true;
  startCallbackDispatched_ = false;
  startTransition_.start(kStartFadeToBlackDuration);
}

void DeflektorishIntroScene::updateLayout() {
  camera_.mScreenSize = screenSize_;
  backgroundCamera_.mScreenSize = screenSize_;
}

glm::vec3 DeflektorishIntroScene::toWorld(glm::vec2 pixels, float z) const {
  const glm::vec2 centered = pixels - Deflektorish::kScreenCenter;
  return {centered.x * kPixelToWorld, centered.y * kPixelToWorld, z};
}

glm::vec3 DeflektorishIntroScene::scaleToWorld(glm::vec2 halfSizePixels) const {
  return {halfSizePixels.x * kPixelToWorld, halfSizePixels.y * kPixelToWorld,
          1.0f};
}
