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
#include <vector>

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
  enum class AiState {
    Idle,
    Walk,
    Run,
  };

  struct FlockAgent {
    MeshNode *node = nullptr;
    glm::vec3 position{0.0f};
    glm::vec3 preCorrectionPosition{0.0f};
    glm::vec3 plannedMoveDelta{0.0f};
    glm::vec3 desiredMoveDirection{0.0f, 0.0f, 1.0f};
    glm::vec3 facingDirection{0.0f, 0.0f, 1.0f};
    AiState desiredState = AiState::Idle;
    float animationBlend = 0.0f;
  };

  [[nodiscard]] std::size_t findClipIndex(std::string_view name,
                                          std::size_t fallback) const;
  std::unique_ptr<MeshNode> createCharacterNode(std::string assetPath,
                                                std::string debugName,
                                                const glm::vec3 &position);
  void initCamera();
  void initLeadCharacter();
  void initFollowers();
  void resolveAnimationClips();
  void updateLeadAnimationBlend(float deltaTime);
  void advanceAi(float deltaTime);
  void advanceFlock(float deltaTime);
  void moveFollowerToSlot(FlockAgent &agent, std::size_t index, float deltaTime);
  void updateFollowerPresentation(FlockAgent &agent, float deltaTime);
  void resolveFlockOverlaps();
  [[nodiscard]] glm::vec3 flockSlotPosition(std::size_t index) const;
  void chooseNextMoveState();
  [[nodiscard]] glm::vec3 separationFromCharacterBounds(
      const glm::vec3 &position, const glm::vec3 &otherPosition) const;
  [[nodiscard]] glm::vec3 separationFromBounds(
      const glm::vec3 &position, const glm::vec3 &otherPosition,
      float halfWidth, float halfDepth) const;
  [[nodiscard]] glm::vec3 currentTarget() const;
  [[nodiscard]] float currentMoveSpeed() const;
  [[nodiscard]] std::string_view aiStateName() const;

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::MeshAssetCache *meshAssetCache_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  glm::vec3 cameraTarget_{0.0f, 0.38f, 0.0f};
  MeshNode *meshNode_ = nullptr;
  std::vector<FlockAgent> chasers_;
  bool debugNormals_ = false;
  bool aiEnabled_ = true;
  AiState aiState_ = AiState::Idle;
  float aiStateTimeRemaining_ = 1.2f;
  std::size_t aiTargetIndex_ = 0;
  std::size_t idleClipIndex_ = 0;
  std::size_t walkClipIndex_ = 0;
  std::size_t sprintClipIndex_ = 0;
  std::size_t locomotionClipIndex_ = 0;
  float locomotionBlendWeight_ = 0.0f;
  float targetLocomotionBlendWeight_ = 0.0f;
  float blendRate_ = 3.0f;
  glm::vec3 characterPosition_{-2.6f, 0.0f, -1.4f};
  glm::vec3 leaderForward_{1.0f, 0.0f, 0.0f};
  glm::vec3 aiTargets_[4] = {{2.6f, 0.0f, -1.0f},
                             {1.45f, 0.0f, 1.65f},
                             {-2.25f, 0.0f, 1.2f},
                             {-2.6f, 0.0f, -1.4f}};
  DL::MeshVisualizerSettings visualizerSettings_;
};
