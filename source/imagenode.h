#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "plane.h"
#include "scenenode.h"
#include "shader.h"
#include <string_view>

class ImageNode : public DL::SceneNode {
public:
  explicit ImageNode(std::string_view glslVersionString, std::string imagePath,
                     basist::etc1_global_selector_codebook *codeBook,
                     DL::SceneNode *parentNode = nullptr);
  ~ImageNode() override = default;
  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> mCamera;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;
  std::string imagePath_;
  basist::etc1_global_selector_codebook *codeBook_;
};
