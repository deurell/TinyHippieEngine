#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "spritebatchrendercomponent.h"

class SpriteBatchNode : public DL::SceneNode {
public:
  SpriteBatchNode(DL::SpriteBatchConfig config, DL::IRenderDevice *renderDevice,
                  DL::RenderResourceCache *renderResourceCache = nullptr,
                  DL::SceneNode *parentNode = nullptr,
                  DL::Camera *camera = nullptr);
  ~SpriteBatchNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SpriteBatchNode";
  }
  [[nodiscard]] const DL::SpriteBatchConfig &config() const { return config_; }

private:
  void initCamera();
  void initComponents();

  DL::SpriteBatchConfig config_;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
};
