#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <memory>
#include <string_view>

namespace DL {
class Light2DVisualizer;
}

class Light2DNode : public DL::SceneNode {
public:
  struct Config {
    glm::vec4 color{1.0f, 0.62f, 0.22f, 1.0f};
    float radius = 1.0f;
    float intensity = 0.55f;
    float softness = 0.75f;
    float flickerAmount = 0.0f;
    float flickerSpeed = 0.0f;
  };

  explicit Light2DNode(DL::IRenderDevice *renderDevice = nullptr,
                       DL::RenderResourceCache *renderResourceCache = nullptr,
                       DL::SceneNode *parentNode = nullptr,
                       DL::Camera *camera = nullptr);
  Light2DNode(Config config, DL::IRenderDevice *renderDevice = nullptr,
              DL::RenderResourceCache *renderResourceCache = nullptr,
              DL::SceneNode *parentNode = nullptr, DL::Camera *camera = nullptr);
  ~Light2DNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "Light2DNode";
  }

  void setConfig(Config config) { config_ = config; }
  [[nodiscard]] const Config &config() const { return config_; }
  void setColor(glm::vec4 color) { config_.color = color; }
  void setRadius(float radius);
  void setIntensity(float intensity);
  void setSoftness(float softness);
  void setFlickerAmount(float amount);
  void setFlickerSpeed(float speed);
  [[nodiscard]] float currentIntensity() const { return currentIntensity_; }

private:
  void initCamera();
  void initComponents();

  Config config_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  glm::vec2 screenSize_{0.0f};
  float currentIntensity_ = 0.55f;
};
