#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <string>

class SpriteNode : public DL::SceneNode {
public:
  explicit SpriteNode(std::string imagePath,
                      basist::etc1_global_selector_codebook *codeBook,
                      DL::IRenderDevice *renderDevice,
                      DL::RenderResourceCache *renderResourceCache = nullptr,
                      DL::SceneNode *parentNode = nullptr,
                      DL::Camera *camera = nullptr);

  ~SpriteNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "SpriteNode";
  }

private:
  void initCamera();
  void initComponents();

  std::string imagePath_;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  glm::vec2 screenSize_{0, 0};
};
