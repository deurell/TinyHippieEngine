#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace DL {
class FogOverlayRenderComponent;
}

class FogOverlayNode : public DL::SceneNode {
public:
  struct Config {
    std::string imagePath;
    glm::vec4 color{0.72f, 0.86f, 0.94f, 0.18f};
    glm::vec2 tiling{2.0f, 1.4f};
    glm::vec2 scrollSpeed{0.015f, 0.004f};
    float alpha = 0.18f;
    float softness = 0.8f;
    float secondLayerStrength = 0.45f;
    glm::vec2 secondLayerScrollSpeed{-0.008f, 0.011f};
    float pulseAmount = 0.04f;
    float pulseSpeed = 0.35f;
  };

  explicit FogOverlayNode(Config config, DL::IRenderDevice *renderDevice,
                          DL::RenderResourceCache *renderResourceCache = nullptr,
                          DL::SceneNode *parentNode = nullptr,
                          DL::Camera *camera = nullptr);
  ~FogOverlayNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "FogOverlayNode";
  }

  [[nodiscard]] const Config &config() const { return config_; }
  void setConfig(Config config) { config_ = std::move(config); }
  void setColor(glm::vec4 color) { config_.color = color; }
  void setAlpha(float alpha);
  void setSoftness(float softness);
  void setTiling(glm::vec2 tiling);
  void setScrollSpeed(glm::vec2 scrollSpeed) {
    config_.scrollSpeed = scrollSpeed;
  }
  void setSecondLayerStrength(float strength);
  void setSecondLayerScrollSpeed(glm::vec2 scrollSpeed) {
    config_.secondLayerScrollSpeed = scrollSpeed;
  }
  void setPulseAmount(float amount);
  void setPulseSpeed(float speed);

private:
  void initCamera();
  void initComponents();

  Config config_;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
};
