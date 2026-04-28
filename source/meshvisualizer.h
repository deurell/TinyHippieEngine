#pragma once

#include "animationplayer.h"
#include "basisu_global_selector_palette.h"
#include "meshasset.h"
#include "meshassetcache.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "skinning.h"
#include "visualizerbase.h"
#include <string_view>
#include <vector>

namespace DL {

struct MeshVisualizerSettings {
  glm::vec3 lightDirection = glm::normalize(glm::vec3(0.35f, 1.0f, 0.25f));
  glm::vec3 lightColor{1.0f, 0.96f, 0.9f};
  float ambientStrength = 0.42f;
  float specularStrength = 0.12f;
  float shininess = 24.0f;
};

struct AnimationBlendState {
  std::size_t baseClipIndex = 0;
  std::size_t blendClipIndex = 0;
  float weight = 0.0f;
  float playbackSpeed = 1.0f;
  bool playing = true;
  bool looping = true;
};

class MeshVisualizer : public VisualizerBase {
public:
  MeshVisualizer(DL::Camera &camera, SceneNode &node,
                 std::shared_ptr<const MeshAsset> asset,
                 basist::etc1_global_selector_codebook *codeBook,
                 DL::IRenderDevice *renderDevice,
                 DL::RenderResourceCache *resourceCache = nullptr,
                 std::string vertexShaderPath = "Shaders/meshnode.vert",
                 std::string fragmentShaderPath = "Shaders/meshnode.frag");
  ~MeshVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "MeshVisualizer";
  }
  void setDebugNormals(bool enabled) { debugNormals_ = enabled; }
  [[nodiscard]] bool debugNormals() const { return debugNormals_; }
  void setSettings(const MeshVisualizerSettings &settings) { settings_ = settings; }
  [[nodiscard]] const MeshVisualizerSettings &settings() const { return settings_; }
  void updateAnimation(float deltaTime);
  void setAnimationPlaying(bool playing) { animationPlayer_.setPlaying(playing); }
  [[nodiscard]] bool isAnimationPlaying() const {
    return animationPlayer_.isPlaying();
  }
  void setAnimationLooping(bool looping) { animationPlayer_.setLooping(looping); }
  [[nodiscard]] bool isAnimationLooping() const {
    return animationPlayer_.isLooping();
  }
  void setAnimationPlaybackSpeed(float speed) {
    animationPlayer_.setPlaybackSpeed(speed);
  }
  [[nodiscard]] float animationPlaybackSpeed() const {
    return animationPlayer_.playbackSpeed();
  }
  [[nodiscard]] std::size_t findAnimationClipIndex(std::string_view name,
                                                   std::size_t fallback = 0u) const;
  void setAnimationClipIndex(std::size_t index) { animationPlayer_.setClipIndex(index); }
  [[nodiscard]] std::size_t animationClipIndex() const {
    return animationPlayer_.clipIndex();
  }
  [[nodiscard]] std::string_view animationClipName(std::size_t index) const;
  [[nodiscard]] bool hasAnimations() const {
    return asset_ != nullptr && !asset_->animations.empty();
  }
  [[nodiscard]] std::size_t animationClipCount() const {
    return asset_ != nullptr ? asset_->animations.size() : 0u;
  }
  [[nodiscard]] float animationTime() const { return animationPlayer_.time(); }
  void applyAnimationBlend(const AnimationBlendState &state);
  void setAnimationBlend(std::size_t baseClipIndex, std::size_t blendClipIndex,
                         float weight);
  void setAnimationBlendByName(std::string_view baseClipName,
                               std::string_view blendClipName, float weight);
  [[nodiscard]] float animationBlendWeight() const { return animationBlendWeight_; }

private:
  struct GpuSubmesh {
    MeshHandle mesh;
    TextureHandle texture;
    glm::vec3 diffuseColor{1.0f, 1.0f, 1.0f};
    glm::vec3 ambientColor{0.4f, 0.4f, 0.4f};
    glm::vec3 specularColor{0.2f, 0.2f, 0.2f};
    float shininess = 16.0f;
    bool hasTexture = false;
    bool sharedTexture = false;
    int sourceNodeIndex = -1;
    int skinIndex = -1;
  };

  TextureHandle createFallbackTexture(bool &sharedTexture);
  TextureHandle loadTexture(const MeshAssetSubmesh &submesh, bool &sharedTexture);
  AnimationPose currentAnimationPose() const;

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  PipelineHandle pipeline_;
  std::shared_ptr<const MeshAsset> asset_;
  std::vector<GpuSubmesh> submeshes_;
  bool debugNormals_ = false;
  MeshVisualizerSettings settings_;
  AnimationPlayer animationPlayer_;
  AnimationPlayer blendAnimationPlayer_;
  float animationBlendWeight_ = 0.0f;
};

} // namespace DL
