#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "textvisualizer.h"
#include <memory>
#include <string_view>

class TextNode : public DL::SceneNode {
public:
  explicit TextNode(DL::SceneNode *parentNode = nullptr,
                    std::string text = "text",
                    DL::IRenderDevice *renderDevice = nullptr,
                    DL::RenderResourceCache *renderResourceCache = nullptr);

  ~TextNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TextNode";
  }
  DL::Camera &getCamera() { return *camera_; }
  DL::TextVisualizer *getTextVisualizer() const { return textVisualizer_; }

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> camera_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  glm::vec2 screenSize_{0, 0};
  std::string text_;
  DL::TextVisualizer *textVisualizer_ = nullptr;
};
