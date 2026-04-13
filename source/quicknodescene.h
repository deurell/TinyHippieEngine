#pragma once
#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>

class PlaneNode;

class QuickNodeScene : public DL::SceneNode {
public:
  explicit QuickNodeScene(DL::IRenderDevice *renderDevice = nullptr,
                          DL::RenderResourceCache *renderResourceCache = nullptr);
  ~QuickNodeScene() override = default;
  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void bounce(double totalTime) const;
  [[nodiscard]] std::unique_ptr<PlaneNode> createPlane(DL::Camera *camera);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_ = nullptr;

  float f1 = 1.1f;
  float f2 = 1.3f;
  float f3 = 0.8f;
  float fOffset = glm::radians(20.0f);
  float fAmp = 20.0f;

  static constexpr unsigned int number_of_planes = 8;
};
