//
// Created by Mikael Deurell on 2021-12-15.
//
#pragma once

#include "camera.h"
#include "iscene.h"
#include "renderdevice.h"

class SimpleScene : public DL::IScene {
public:
  explicit SimpleScene(DL::IRenderDevice *renderDevice);
  ~SimpleScene() override;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Camera> mCamera;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::MeshHandle mesh_;
  DL::PipelineHandle pipeline_;
  glm::vec2 mScreenSize{0, 0};
};
