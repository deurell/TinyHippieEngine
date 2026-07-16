#pragma once

#include "camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "renderdevice.h"
#include "renderqueue.h"
#include "renderresourcecache.h"
#include "stb_truetype.h"
#include "visualizerbase.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace DL {

enum class TextAlignment { LEFT, CENTER, RIGHT };
enum class TextAnchor {
  TOP_LEFT,
  TOP_CENTER,
  TOP_RIGHT,
  CENTER_LEFT,
  CENTER,
  CENTER_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_CENTER,
  BOTTOM_RIGHT
};

struct TextGlyphInfo {
  glm::vec3 positions[4];
  glm::vec2 uvs[4];
  float offsetX = 0;
  float offsetY = 0;
};

struct FontData {
  TextureHandle texture;
  std::shared_ptr<stbtt_packedchar[]> fontInfo;
  float fontScale = 1.0f;
  float fontSize = 0.0f;
};

class TextVisualizer : public VisualizerBase {
public:
  explicit TextVisualizer(DL::Camera &camera, SceneNode &node, std::string text,
                          const std::string &fontPath,
                          DL::IRenderDevice *renderDevice,
                          DL::RenderResourceCache *resourceCache,
                          std::string vertexShaderPath,
                          std::string fragmentShaderPath,
                          float pixelHeight = 48.0f);

  ~TextVisualizer() override;
  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "TextVisualizer";
  }
  void setText(std::string text);
  void setAlignment(TextAlignment alignment);
  void setAnchor(TextAnchor anchor);
  void setLayoutWidth(float width);
  void setFontPixelHeight(float pixelHeight);
  void setTextColor(glm::vec4 color) { textColor_ = color; }
  void setShadowColor(glm::vec4 color) { shadowColor_ = color; }
  void setShadowOffset(glm::vec2 offset) { shadowOffset_ = offset; }

  float rotAngle1_ = 0.04f;
  float rotAngle2_ = 0.5f;
  float color1_ = 0.03f;
  float color2_ = 1.35f;

private:
  bool loadFontTexture(std::string_view fontPath);
  bool acquireFontTexture();
  [[nodiscard]] std::uint32_t atlasSizeForPixelHeight() const;
  TextGlyphInfo makeGlyphInfo(std::uint32_t codepoint, float offsetX,
                              float offsetY);
  void initGraphics();
  void destroyMesh();

  std::string text_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  TextureHandle fontTexture_;
  PipelineHandle pipeline_;
  TextAlignment alignment_ = TextAlignment::CENTER;
  TextAnchor anchor_ = TextAnchor::CENTER;
  float layoutWidth_ = 0.0f;
  const float kerning_ = 2.0f;

  float desiredPixelHeight_ = 48.0f;
  float fontScale_ = 1.0f;
  float fontSize_ = 0.0f;
  glm::vec4 textColor_{1.0f, 1.0f, 1.0f, 1.0f};
  glm::vec4 shadowColor_{0.0f, 0.0f, 0.0f, 0.58f};
  glm::vec2 shadowOffset_{1.5f, -1.5f};

  std::uint32_t fontAtlasWidth_ = 1024;
  std::uint32_t fontAtlasHeight_ = 1024;
  const uint32_t fontOversampleX_ = 2;
  const uint32_t fontOversampleY_ = 2;
  std::shared_ptr<stbtt_packedchar[]> fontCharInfo_;
  std::string fontPath_;
  bool sharedFontTexture_ = false;
  Bounds localBounds_;
};

} // namespace DL
