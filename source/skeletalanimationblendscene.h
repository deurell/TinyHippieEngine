#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "meshnode.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>
#include <string_view>

class SkeletalAnimationBlendScene : public DL::SceneNode {
public:
  explicit SkeletalAnimationBlendScene(
      DL::IRenderDevice *renderDevice = nullptr,
      basist::etc1_global_selector_codebook *codeBook = nullptr,
      DL::MeshAssetCache *meshAssetCache = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr);
  ~SkeletalAnimationBlendScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SkeletalAnimationBlendScene";
  }

private:
  [[nodiscard]] std::size_t findClipIndex(std::string_view name,
                                          std::size_t fallback) const;
  void resolveAnimationClips();
  void applyAnimationBlend();

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  glm::vec3 cameraTarget_{0.0f, 2.5f, 0.0f};
  MeshNode *meshNode_ = nullptr;
  bool debugNormals_ = false;
  bool autoCycle_ = true;
  bool walking_ = false;
  std::size_t idleClipIndex_ = 0;
  std::size_t walkClipIndex_ = 0;
  float walkBlendWeight_ = 0.0f;
  float targetWalkBlendWeight_ = 0.0f;
  float blendRate_ = 3.5f;
  float walkDistance_ = -2.6f;
  DL::MeshVisualizerSettings visualizerSettings_;
};
