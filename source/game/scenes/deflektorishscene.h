#pragma once

#include "cameranode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "shaderplanenode.h"
#include <functional>
#include <glm/glm.hpp>
#include <string_view>
#include <vector>

class DeflektorishScene final : public DL::SceneNode {
public:
  explicit DeflektorishScene(
      DL::IRenderDevice *renderDevice = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr,
      std::function<void(glm::vec2, float)> postBumpCallback = {});
  ~DeflektorishScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "DeflektorishScene";
  }

private:
  struct BeamSegment {
    DL::ShaderPlaneNode *node = nullptr;
    float energy = 0.0f;
  };

  struct Reflektor {
    glm::vec2 position{0.0f};
    float angle = 0.0f;
    bool automatic = false;
    float speed = 0.0f;
    float glow = 0.0f;
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct Target {
    glm::vec2 position{0.0f};
    bool alive = true;
    float dying = 0.0f;
    float hitEnergy = 0.0f;
    float hitFlash = 0.0f;
    float phase = 0.0f;
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct Blocker {
    glm::vec2 position{0.0f};
    bool reflective = false;
    float glow = 0.0f;
    float energy = 0.0f;
    glm::vec2 hitPoint{0.0f};
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct Explosion {
    DL::ShaderPlaneNode *node = nullptr;
    bool active = false;
    glm::vec2 position{0.0f};
    float time = 0.0f;
    float duration = 0.74f;
    float energy = 0.0f;
    float seed = 0.0f;
  };

  struct Portal {
    glm::vec2 entryPosition{0.0f};
    glm::vec2 exitPosition{0.0f};
    float phase = 0.0f;
    float glow = 0.0f;
    glm::vec2 entryHitPoint{0.0f};
    glm::vec2 exitHitPoint{0.0f};
    DL::ShaderPlaneNode *entryNode = nullptr;
    DL::ShaderPlaneNode *exitNode = nullptr;
  };

  struct Filter {
    glm::vec2 position{0.0f};
    float angle = 0.0f;
    bool automatic = false;
    float speed = 0.0f;
    float passGlow = 0.0f;
    float blockGlow = 0.0f;
    glm::vec2 hitPoint{0.0f};
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct Splitter {
    glm::vec2 position{0.0f};
    float angle = 0.0f;
    float glow = 0.0f;
    glm::vec2 hitPoint{0.0f};
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct BeamResult {
    std::vector<bool> activeReflektors;
    std::vector<float> reflektorEnergy;
    std::vector<bool> activeBlockers;
    std::vector<float> blockerEnergy;
    std::vector<glm::vec2> blockerHit;
    std::vector<bool> blockerHasHit;
    std::vector<bool> hitTargets;
    std::vector<float> targetEnergy;
    std::vector<bool> activePortals;
    std::vector<glm::vec2> portalEntryHit;
    std::vector<glm::vec2> portalExitHit;
    std::vector<bool> portalHasHit;
    std::vector<bool> passingFilters;
    std::vector<bool> blockedFilters;
    std::vector<glm::vec2> filterHit;
    std::vector<bool> filterHasHit;
    std::vector<bool> activeSplitters;
    std::vector<glm::vec2> splitterHit;
    std::vector<bool> splitterHasHit;
  };

  DL::ShaderPlaneNode *addShaderPlane(std::string name, int proceduralStyle,
                                      DL::BlendMode blendMode,
                                      glm::vec2 position, glm::vec2 halfSize,
                                      int renderLayer, float z = 0.0f,
                                      float rotationRadians = 0.0f);
  void createCameraNode();
  void addBackground();
  void spawnLevel();
  void updateInput(const DL::FrameContext &ctx);
  void updateReflektors(float dt);
  void updateFilters(float dt);
  void updateSelection(float dt);
  BeamResult solveBeam();
  void updateSource(float dt, const BeamResult &result);
  void updateReflektorVisuals(float dt, const BeamResult &result);
  void updateBlockerVisuals(float dt, const BeamResult &result);
  void updatePortalVisuals(float dt, const BeamResult &result);
  void updateFilterVisuals(float dt, const BeamResult &result);
  void updateSplitterVisuals(float dt, const BeamResult &result);
  void updateTargets(float dt, const BeamResult &result);
  void spawnExplosion(glm::vec2 position, float energy);
  void updateExplosions(float dt);
  void updateCameraShake(float dt);
  void startCameraShake(float strength, float duration);
  void layoutSegment(BeamSegment &segment, glm::vec2 start, glm::vec2 end,
                     float energy);
  void hideSegment(BeamSegment &segment);
  glm::vec2 screenToWorld(glm::vec2 screenPosition) const;
  bool selectReflektorAtWorld(glm::vec2 worldPosition);
  int findNextManualReflektor(int startIndex) const;

  static glm::vec2 grid(int x, int y);
  static glm::vec2 toWorld(glm::vec2 pixels);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::function<void(glm::vec2, float)> postBumpCallback_;
  DL::CameraNode *cameraNode_ = nullptr;
  glm::vec2 screenSize_{0.0f};
  glm::vec2 framebufferSize_{0.0f};

  std::vector<BeamSegment> segments_;
  std::vector<Reflektor> reflektors_;
  std::vector<Target> targets_;
  std::vector<Blocker> blockers_;
  std::vector<Explosion> explosions_;
  std::vector<Portal> portals_;
  std::vector<Filter> filters_;
  std::vector<Splitter> splitters_;
  DL::ShaderPlaneNode *source_ = nullptr;
  DL::ShaderPlaneNode *selection_ = nullptr;
  int selectedReflektor_ = -1;
  bool previousLeftMouseDown_ = false;
  bool previousSelectNextDown_ = false;
  float rotateInput_ = 0.0f;
  float sourcePulse_ = 0.0f;
  float sourceLoad_ = 0.0f;
  float sourceLoadTarget_ = 0.0f;
  float selectionFlash_ = 0.0f;
  float shakeTrauma_ = 0.0f;
  float shakeKick_ = 0.0f;
  float shakeSeed_ = 1.7f;
  float shakeKickAngle_ = 0.0f;
  float elapsed_ = 0.0f;
};
