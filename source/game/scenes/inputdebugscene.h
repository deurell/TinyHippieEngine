#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <string_view>

class PlaneNode;
class TextNode;

class InputDebugScene final : public DL::SceneNode {
public:
  explicit InputDebugScene(
      DL::IRenderDevice *renderDevice = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr);
  ~InputDebugScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "InputDebugScene";
  }

private:
  TextNode *addText(std::string text, glm::vec3 position, float pixelHeight);
  PlaneNode *addPlane(std::string name, glm::vec3 position, glm::vec3 scale,
                      glm::vec4 color, int renderLayer);
  void updateReadout(const DL::FrameContext &ctx);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  DL::Camera camera_{glm::vec3(0.0f, 0.0f, 10.0f)};
  PlaneNode *axisDot_ = nullptr;
  PlaneNode *fireIndicator_ = nullptr;
  TextNode *axisText_ = nullptr;
  TextNode *fireText_ = nullptr;
  glm::vec2 screenSize_{0.0f};
  glm::vec2 framebufferSize_{0.0f};
};
