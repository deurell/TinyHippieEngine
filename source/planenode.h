//
// Created by Mikael Deurell on 2023-08-16.
//

#pragma once

#include "camera.h"
#include "plane.h"
#include "renderdevice.h"
#include "scenenode.h"
#include "shader.h"
#include <string_view>

class PlaneNode : public DL::SceneNode {
public:
  enum class PlaneType { Simple, Spinner };

  explicit PlaneNode(std::string_view glslVersionString,
                     DL::SceneNode *parentNode = nullptr,
                     DL::Camera *camera = nullptr,
                     DL::IRenderDevice *renderDevice = nullptr);

  ~PlaneNode() override = default;
  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  PlaneType planeType = PlaneType::Simple;
  glm::vec4 color{0.9f, 0.9f, 0.9f, 1.0f};

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  glm::vec2 screenSize_{0, 0};
  std::string glslVersionString_;
};
