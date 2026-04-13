#pragma once

#include "phongshapenode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"

class PhongShapeScene : public DL::SceneNode {
public:
  explicit PhongShapeScene(DL::IRenderDevice *renderDevice = nullptr,
                           DL::RenderResourceCache *renderResourceCache = nullptr);
  ~PhongShapeScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  PhongShapeNode *cubeNode_ = nullptr;
  PhongShapeNode *sphereNode_ = nullptr;
  PhongShapeNode *cylinderNode_ = nullptr;
};
