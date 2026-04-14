//
// Created by Mikael Deurell on 2023-08-16.
//

#pragma once

#include "camera.h"
#include "planevisualizer.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include <string_view>

class PlaneNode : public DL::SceneNode {
public:
  enum class PlaneType { Simple, Spinner };

  explicit PlaneNode(DL::SceneNode *parentNode = nullptr,
                     DL::Camera *camera = nullptr,
                     DL::IRenderDevice *renderDevice = nullptr,
                     DL::RenderResourceCache *renderResourceCache = nullptr);

  ~PlaneNode() override = default;
  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PlaneNode";
  }
  PlaneType planeType = PlaneType::Simple;
  glm::vec4 color{0.9f, 0.9f, 0.9f, 1.0f};

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  DL::PlaneVisualizer *planeVisualizer_ = nullptr;
  glm::vec2 screenSize_{0, 0};
};
