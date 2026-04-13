#pragma once

#include "basisu_global_selector_palette.h"
#include "camera.h"
#include "meshnode.h"
#include "meshvisualizer.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <memory>

class GltfNodeScene : public DL::SceneNode {
public:
  explicit GltfNodeScene(DL::IRenderDevice *renderDevice = nullptr,
                         basist::etc1_global_selector_codebook *codeBook = nullptr);
  ~GltfNodeScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  std::unique_ptr<DL::Camera> camera_;
  glm::vec3 cameraTarget_{0.0f, 2.5f, 0.0f};
  MeshNode *meshNode_ = nullptr;
  bool debugNormals_ = false;
  DL::MeshVisualizerSettings visualizerSettings_;
};
