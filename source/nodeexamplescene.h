//
// Created by Mikael Deurell on 2023-08-18.
//

#pragma once
#include "basisu_global_selector_palette.h"
#include "planenode.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "spritenode.h"
#include "textnode.h"

class NodeExampleScene : public DL::SceneNode {
public:
  explicit NodeExampleScene(
      DL::IRenderDevice *renderDevice = nullptr,
      basist::etc1_global_selector_codebook *codeBook = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr);
  ~NodeExampleScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void wrapScrollText();

  static constexpr glm::vec3 INITIAL_TEXT_POSITION = {0, -24, 0};
  static constexpr float TEXT_RESET_POSITION = 100;

  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  PlaneNode *plane1_ = nullptr;
  PlaneNode *plane2_ = nullptr;
  SpriteNode *spriteNode_ = nullptr;
  
  TextNode *textNode_ = nullptr;
  float scale_ = 1.0f;
  float rotation_ = 0.0f;
  float scrollAngle = 0;
  float scrollSpeed = 0.04f;
  glm::vec2 screenSize_{0, 0};
};
