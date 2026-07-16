#pragma once

#include "camera.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "textrendercomponent.h"
#include <memory>
#include <string_view>

class TextNode : public DL::SceneNode {
public:
  explicit TextNode(DL::SceneNode *parentNode = nullptr,
                    std::string text = "text",
                    DL::IRenderDevice *renderDevice = nullptr,
                    DL::RenderResourceCache *renderResourceCache = nullptr,
                    DL::Camera *camera = nullptr);

  ~TextNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TextNode";
  }
  DL::Camera &getCamera() { return *camera_; }
  DL::TextRenderComponent *getTextRenderComponent() const { return textRenderComponent_; }
  [[nodiscard]] std::string_view text() const { return text_; }
  void setText(std::string text);
  void setBillboardEnabled(bool enabled) { billboardEnabled_ = enabled; }
  [[nodiscard]] bool billboardEnabled() const { return billboardEnabled_; }
  void setTextAlignment(DL::TextAlignment alignment);
  [[nodiscard]] DL::TextAlignment textAlignment() const { return alignment_; }
  void setTextAnchor(DL::TextAnchor anchor);
  [[nodiscard]] DL::TextAnchor textAnchor() const { return anchor_; }
  void setFontPixelHeight(float pixelHeight);
  [[nodiscard]] float fontPixelHeight() const { return fontPixelHeight_; }
  void setTextColor(glm::vec4 color);
  [[nodiscard]] glm::vec4 textColor() const { return textColor_; }
  void setShadowColor(glm::vec4 color);
  [[nodiscard]] glm::vec4 shadowColor() const { return shadowColor_; }
  void setShadowOffset(glm::vec2 offset);
  [[nodiscard]] glm::vec2 shadowOffset() const { return shadowOffset_; }

private:
  void initCamera();
  void initComponents();
  void updateBillboardRotation();

  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  glm::vec2 screenSize_{0, 0};
  std::string text_;
  DL::TextRenderComponent *textRenderComponent_ = nullptr;
  bool billboardEnabled_ = false;
  DL::TextAlignment alignment_ = DL::TextAlignment::CENTER;
  DL::TextAnchor anchor_ = DL::TextAnchor::CENTER;
  float fontPixelHeight_ = 48.0f;
  glm::vec4 textColor_{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 shadowColor_{0.0f, 0.0f, 0.0f, 0.58f};
  glm::vec2 shadowOffset_{1.5f, -1.5f};
};
