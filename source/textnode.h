#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "scenenode.h"
#include <memory>
#include <string_view>

class TextNode : public DL::SceneNode {
public:
  explicit TextNode(DL::SceneNode *parentNode = nullptr,
                    std::string text = "text",
                    DL::IRenderDevice *renderDevice = nullptr);

  ~TextNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  DL::Camera &getCamera() { return *camera_; }

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> camera_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  glm::vec2 screenSize_{0, 0};
  std::string text_;
};
