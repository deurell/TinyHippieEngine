#pragma once

#include "camera.h"
#include "game/deflektorish/beamworld.h"
#include "game/deflektorish/deflektorishrenderer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "shaderplanenode.h"
#include "textnode.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

class DeflektorishIntroScene final : public DL::SceneNode {
public:
  explicit DeflektorishIntroScene(
      DL::IRenderDevice *renderDevice = nullptr,
      DL::RenderResourceCache *renderResourceCache = nullptr,
      std::function<void()> startCallback = {});
  ~DeflektorishIntroScene() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  void onFramebufferSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "DeflektorishIntroScene";
  }

private:
  struct IntroReflektor {
    glm::vec2 basePosition{0.0f};
    glm::vec2 position{0.0f};
    float angle = 0.0f;
    float baseAngle = 0.0f;
    float previousAngle = 0.0f;
    float aimFlash = 0.0f;
    float speed = 1.0f;
    float phase = 0.0f;
    float glow = 0.0f;
    DL::ShaderPlaneNode *node = nullptr;
  };

  struct IntroSource {
    glm::vec2 basePosition{0.0f};
    glm::vec2 position{0.0f};
    float angle = 0.0f;
    float baseAngle = 0.0f;
    float phase = 0.0f;
    DL::ShaderPlaneNode *node = nullptr;
  };

  TextNode *addText(std::string text, glm::vec2 positionPixels,
                    float pixelHeight, glm::vec4 color, int renderLayer);
  TextNode *addLeftText(std::string text, glm::vec2 positionPixels,
                        float pixelHeight, glm::vec4 color, int renderLayer);
  DL::ShaderPlaneNode *addPlane(std::string name, int style,
                                DL::BlendMode blendMode,
                                glm::vec2 positionPixels,
                                glm::vec2 halfSizePixels, int renderLayer,
                                float z = 0.0f,
                                float rotationRadians = 0.0f);
  DL::ShaderPlaneNode *addPlaneForCamera(std::string name, int style,
                                         DL::BlendMode blendMode,
                                         glm::vec2 positionPixels,
                                         glm::vec2 halfSizePixels,
                                         int renderLayer, DL::Camera *camera,
                                         float z = 0.0f,
                                         float rotationRadians = 0.0f);
  void addHighScores();
  void addCredits();
  void createLiveShowcase();
  void updateBackgroundCamera();
  void updateLiveShowcase(float dt);
  void updateAttractPage();
  void requestStart();
  void updateLayout();
  glm::vec3 toWorld(glm::vec2 pixels, float z = 0.0f) const;
  glm::vec3 scaleToWorld(glm::vec2 halfSizePixels) const;

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::function<void()> startCallback_;
  DL::Camera camera_{glm::vec3(0.0f, 0.0f, 10.0f)};
  DL::Camera backgroundCamera_{glm::vec3(0.0f, 0.0f, 10.0f)};
  glm::vec2 screenSize_{0.0f};
  glm::vec2 framebufferSize_{0.0f};
  std::vector<TextNode *> highScoreTexts_;
  std::vector<TextNode *> creditTexts_;
  Deflektorish::Renderer renderer_;
  std::vector<IntroSource> demoSources_;
  std::vector<IntroReflektor> demoReflektors_;
  TextNode *pressFire_ = nullptr;
  bool previousFireDown_ = false;
  bool startRequested_ = false;
  float elapsed_ = 0.0f;
};
