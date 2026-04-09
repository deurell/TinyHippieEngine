//
// Created by Mikael Deurell on 2024-08-22.
//
#pragma once
#include "basisu_global_selector_palette.h"
#include "renderdevice.h"
#include "scenenode.h"

class ImageNode;
class PlaneNode;

namespace DL {
class GlosifyScene : public DL::SceneNode {
public:
  explicit GlosifyScene(std::string glslVersionString,
                        basist::etc1_global_selector_codebook *codeBook,
                        DL::IRenderDevice *renderDevice);
  ~GlosifyScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<PlaneNode> createPlane(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);
  std::unique_ptr<ImageNode> createImage(glm::vec3 position, glm::vec3 scale,
                                         glm::quat rotation);

  std::string glslVersionString_;
  PlaneNode *plane_node_ = nullptr;
  ImageNode *image_node_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_;
  DL::IRenderDevice *renderDevice_ = nullptr;
};
} // namespace DL
