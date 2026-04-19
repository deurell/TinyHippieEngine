#pragma once

#include "audiosystem.h"
#include "phongshapenode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>
#include <vector>

class SpectrumAnalyzerScene : public DL::SceneNode {
public:
  explicit SpectrumAnalyzerScene(
      DL::IRenderDevice *renderDevice = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr,
      DL::AudioSystem *audioSystem = nullptr);
  ~SpectrumAnalyzerScene() override;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SpectrumAnalyzerScene";
  }

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  DL::AudioSystem *audioSystem_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  std::vector<PhongShapeNode *> bars_;
  std::vector<float> barXPositions_;
  DL::AudioSystem::SoundId musicLoopId_ = DL::AudioSystem::kInvalidSoundId;
  static constexpr const char *kSpectrumTrackName = "spectrum_track";
};
