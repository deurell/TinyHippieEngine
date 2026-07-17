#pragma once

#include "camera.h"
#include "planerendercomponent.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <memory>
#include <string>
#include <string_view>

namespace DL {

class ShaderPlaneNode final : public SceneNode {
public:
  struct Config {
    std::string vertexShader = "Shaders/simple.vert";
    std::string fragmentShader = "Shaders/simple.frag";
    BlendMode blendMode = BlendMode::Alpha;
    bool depthTest = false;
    int proceduralStyle = 0;
    glm::vec4 color{1.0f};
    glm::vec4 params0{0.0f};
    glm::vec4 params1{0.0f};
  };

  explicit ShaderPlaneNode(SceneNode *parentNode = nullptr,
                           Camera *camera = nullptr,
                           IRenderDevice *renderDevice = nullptr,
                           RenderResourceCache *renderResourceCache = nullptr);
  explicit ShaderPlaneNode(Config config, SceneNode *parentNode = nullptr,
                           Camera *camera = nullptr,
                           IRenderDevice *renderDevice = nullptr,
                           RenderResourceCache *renderResourceCache = nullptr);

  void init() override;
  void update(const FrameContext &ctx) override;
  void render(const FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "ShaderPlaneNode";
  }

  Config config;

private:
  void initCamera();
  void initComponent();

  std::unique_ptr<Camera> localCamera_;
  Camera *camera_ = nullptr;
  IRenderDevice *renderDevice_ = nullptr;
  RenderResourceCache *renderResourceCache_ = nullptr;
  PlaneRenderComponent *planeRenderComponent_ = nullptr;
};

} // namespace DL
