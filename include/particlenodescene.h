#pragma once

#include "camera.h"
#include "particlesystemnode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <memory>

class ParticleNodeScene : public DL::SceneNode {
public:
  explicit ParticleNodeScene(DL::IRenderDevice *renderDevice,
                             DL::RenderResourceCache *renderResourceCache = nullptr);
  ~ParticleNodeScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;

  static constexpr int number_of_particles = 64;

private:
  glm::vec3 screenToWorld(double x, double y, float worldZ) const;

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  ParticleSystemNode *particleSystemNode_ = nullptr;
  glm::vec2 screenSize_{0.0f, 0.0f};
  glm::vec2 framebufferSize_{0.0f, 0.0f};
};
