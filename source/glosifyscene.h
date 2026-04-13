//
// Created by Mikael Deurell on 2024-08-22.
//
#pragma once
#include "basisu_global_selector_palette.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"

class PlaneNode;
class SpriteNode;

namespace DL {
class GlosifyScene : public DL::SceneNode {
public:
  explicit GlosifyScene(basist::etc1_global_selector_codebook *codeBook,
                        DL::IRenderDevice *renderDevice,
                        DL::RenderResourceCache *renderResourceCache = nullptr);
  ~GlosifyScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);
  std::unique_ptr<SpriteNode> createSprite(glm::vec3 position, glm::vec3 scale,
                                           glm::quat rotation);

  PlaneNode *plane_node_ = nullptr;
  SpriteNode *sprite_node_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
};
} // namespace DL
