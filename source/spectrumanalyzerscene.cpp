#include "spectrumanalyzerscene.h"

#include "debugui.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace {
glm::vec3 lerpColor(const glm::vec3 &a, const glm::vec3 &b, float t) {
  return a + (b - a) * t;
}
} // namespace

SpectrumAnalyzerScene::SpectrumAnalyzerScene(
    DL::IRenderDevice *renderDevice,
    DL::RenderResourceCache *renderResourceCache,
    DL::AudioSystem *audioSystem)
    : SceneNode(nullptr), renderDevice_(renderDevice),
      renderResourceCache_(renderResourceCache), audioSystem_(audioSystem) {}

SpectrumAnalyzerScene::~SpectrumAnalyzerScene() {
  if (audioSystem_ != nullptr &&
      musicLoopId_ != DL::AudioSystem::kInvalidSoundId) {
    audioSystem_->stop(musicLoopId_);
    musicLoopId_ = DL::AudioSystem::kInvalidSoundId;
  }
}

void SpectrumAnalyzerScene::init() {
  SceneNode::init();
  camera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 6.0f, 18.0f));
  camera_->lookAt({0.0f, 2.0f, 0.0f});

  const std::size_t barCount = DL::AudioSystem::kSpectrumBandCount;
  bars_.clear();
  barXPositions_.clear();
  bars_.reserve(barCount);
  barXPositions_.reserve(barCount);

  const float spacing = 0.52f;
  const float startX = -0.5f * static_cast<float>(barCount - 1) * spacing;
  for (std::size_t i = 0; i < barCount; ++i) {
    auto bar = std::make_unique<PhongShapeNode>(
        ShapeType::Cube, renderDevice_, renderResourceCache_, this,
        camera_.get());
    const float t =
        barCount > 1 ? static_cast<float>(i) / static_cast<float>(barCount - 1)
                     : 0.0f;
    bar->setMaterial({.diffuse =
                          lerpColor({0.16f, 0.58f, 0.96f},
                                    {0.97f, 0.42f, 0.24f}, t),
                      .ambient =
                          lerpColor({0.08f, 0.18f, 0.28f},
                                    {0.25f, 0.12f, 0.10f}, t),
                      .specular = {0.35f, 0.35f, 0.35f},
                      .shininess = 18.0f});
    bar->init();
    const float x = startX + static_cast<float>(i) * spacing;
    bar->setLocalScale({0.35f, 0.12f, 0.35f});
    bar->setLocalPosition({x, 0.06f, 0.0f});
    bar->setDebugName("Band " + std::to_string(i));
    barXPositions_.push_back(x);
    bars_.push_back(bar.get());
    addChild(std::move(bar));
  }

  if (audioSystem_ != nullptr) {
    audioSystem_->loadClip(kSpectrumTrackName, "Resources/gods.mp3", 0);
    audioSystem_->loadClip("spectrum_hit", "Resources/unlock.wav", 8);
    musicLoopId_ =
        audioSystem_->playLoop(kSpectrumTrackName, DL::AudioGroup::Music, 0.75f);
  }
}

void SpectrumAnalyzerScene::update(const DL::FrameContext &ctx) {
  const std::array<float, DL::AudioSystem::kSpectrumBandCount> emptySpectrum{};
  const auto &spectrum =
      audioSystem_ != nullptr ? audioSystem_->spectrumBands() : emptySpectrum;

  for (std::size_t i = 0; i < bars_.size(); ++i) {
    const float level = std::clamp(spectrum[i], 0.0f, 1.6f);
    const float height = 0.12f + level * 5.2f;
    const float zOffset =
        std::sin(static_cast<float>(ctx.total_time) * 0.8f +
                 static_cast<float>(i) * 0.17f) *
        0.08f;
    bars_[i]->setLocalScale({0.35f, height, 0.35f});
    bars_[i]->setLocalPosition({barXPositions_[i], height * 0.5f, zOffset});
  }

  SceneNode::update(ctx);
}

void SpectrumAnalyzerScene::render(const DL::FrameContext &ctx) {
  if (renderDevice_ != nullptr) {
    renderDevice_->beginFrame({.clearColor = {0.04f, 0.05f, 0.08f, 1.0f},
                               .clearFlags = DL::ClearFlags::ColorDepth,
                               .depthMode = DL::DepthMode::Less});
  }

#ifdef USE_IMGUI
  DL::beginDebugUiFrame();
  if (ImGui::Begin("Spectrum Scene")) {
    ImGui::Text("Music: Resources/gods.mp3");
    ImGui::Text("Click or press Enter to trigger SFX.");
  }
  ImGui::End();
#endif

  SceneNode::render(ctx);
}

void SpectrumAnalyzerScene::onClick(double /*x*/, double /*y*/) {
  if (audioSystem_ != nullptr) {
    audioSystem_->playOneShot("spectrum_hit", DL::AudioGroup::SFX, 1.0f);
  }
}

void SpectrumAnalyzerScene::onKey(int key) {
  if (audioSystem_ == nullptr) {
    return;
  }
  if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
    audioSystem_->playOneShot("spectrum_hit", DL::AudioGroup::SFX, 1.0f);
  }
}

void SpectrumAnalyzerScene::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}
