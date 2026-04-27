#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshassetcache.h"
#include "meshasset.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>

class MeshNode : public DL::SceneNode {
public:
  explicit MeshNode(std::string assetPath,
                    basist::etc1_global_selector_codebook *codeBook,
                    DL::IRenderDevice *renderDevice,
                    DL::MeshAssetCache *meshAssetCache = nullptr,
                    DL::RenderResourceCache *renderResourceCache = nullptr,
                    DL::SceneNode *parentNode = nullptr,
                    DL::Camera *camera = nullptr);
  ~MeshNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "MeshNode";
  }
  void setDebugNormals(bool enabled);
  [[nodiscard]] bool debugNormals() const { return debugNormals_; }
  void setVisualizerSettings(const DL::MeshVisualizerSettings &settings);
  [[nodiscard]] const DL::MeshVisualizerSettings &visualizerSettings() const {
    return visualizerSettings_;
  }
  void setAnimationPlaying(bool playing);
  [[nodiscard]] bool isAnimationPlaying() const;
  void setAnimationLooping(bool looping);
  [[nodiscard]] bool isAnimationLooping() const;
  void setAnimationPlaybackSpeed(float speed);
  [[nodiscard]] float animationPlaybackSpeed() const;
  void setAnimationClipIndex(std::size_t index);
  [[nodiscard]] std::size_t animationClipIndex() const;
  [[nodiscard]] std::string_view animationClipName(std::size_t index) const;
  [[nodiscard]] std::size_t animationClipCount() const;
  [[nodiscard]] bool hasAnimations() const;
  void setAnimationBlend(std::size_t baseClipIndex, std::size_t blendClipIndex,
                         float weight);
  [[nodiscard]] float animationBlendWeight() const;

private:
  void initCamera();
  void initComponents();

  std::string assetPath_;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  glm::vec2 screenSize_{0.0f, 0.0f};
  DL::MeshVisualizer *meshVisualizer_ = nullptr;
  bool debugNormals_ = false;
  DL::MeshVisualizerSettings visualizerSettings_;
};
