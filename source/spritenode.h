#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <string>

namespace DL {
class SpriteVisualizer;
}

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
  void setBillboardEnabled(bool enabled) { billboardEnabled_ = enabled; }
  [[nodiscard]] bool billboardEnabled() const { return billboardEnabled_; }
  void setAtlasSourceRectPixels(const glm::vec4 &rect);
  [[nodiscard]] const glm::vec4 &atlasSourceRectPixels() const {
    return atlasSourceRectPixels_;
  }
  void setAtlasFlip(bool flipX, bool flipY, bool flipDiagonal = false);
  [[nodiscard]] glm::bvec3 atlasFlip() const { return atlasFlip_; }

private:
  void initCamera();
  void initComponents();
  void updateBillboardRotation();

  std::string imagePath_;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  glm::vec2 screenSize_{0, 0};
  bool billboardEnabled_ = false;
  glm::vec4 atlasSourceRectPixels_{0.0f, 0.0f, -1.0f, -1.0f};
  glm::bvec3 atlasFlip_{false, false, false};
  DL::SpriteVisualizer *spriteVisualizer_ = nullptr;
};
