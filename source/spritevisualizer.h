#pragma once

#include "basisu_global_selector_palette.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "visualizerbase.h"
#include <glm/glm.hpp>
#include <string>

namespace DL {

class SpriteVisualizer : public VisualizerBase {
public:
  explicit SpriteVisualizer(
      std::string name, DL::Camera &camera, SceneNode &node,
      std::string texturePath,
      basist::etc1_global_selector_codebook *codeBook,
      DL::IRenderDevice *renderDevice,
      DL::RenderResourceCache *resourceCache = nullptr,
      std::string vertexShaderPath = "Shaders/image.vert",
      std::string fragmentShaderPath = "Shaders/image.frag");

  ~SpriteVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;

private:
  bool loadTexture();

  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  TextureHandle texture_;
  PipelineHandle pipeline_;
  std::string texturePath_;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
};

} // namespace DL
