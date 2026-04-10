#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <string_view>

class ImageNode : public DL::SceneNode {
public:
  explicit ImageNode(std::string_view glslVersionString, std::string imagePath,
                     basist::etc1_global_selector_codebook *codeBook,
                     DL::IRenderDevice *renderDevice,
                     DL::SceneNode *parentNode = nullptr);
  ~ImageNode() override = default;
  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> mCamera;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;
  std::string imagePath_;
  basist::etc1_global_selector_codebook *codeBook_;
  DL::IRenderDevice *renderDevice_ = nullptr;
};
