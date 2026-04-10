#pragma once

#include "camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "renderdevice.h"
#include "stb_truetype.h"
#include "visualizerbase.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace DL {

enum class TextAlignment { LEFT, CENTER };

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
  std::uint32_t refCount = 0;
};

class TextVisualizer : public VisualizerBase {
public:
  explicit TextVisualizer(std::string name, DL::Camera &camera,
                          SceneNode &node, std::string text,
                          const std::string &fontPath,
                          DL::IRenderDevice *renderDevice,
                          std::string vertexShaderPath,
                          std::string fragmentShaderPath);

  ~TextVisualizer() override;
  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;
  void setText(std::string text);
  void setAlignment(TextAlignment alignment);
  void setLayoutWidth(float width);

  float rotAngle1_ = 0.04f;
  float rotAngle2_ = 0.5f;
  float color1_ = 0.03f;
  float color2_ = 1.35f;

private:
  bool loadFontTexture(std::string_view fontPath);
  TextGlyphInfo makeGlyphInfo(char character, float offsetX, float offsetY);
  void initGraphics();
  void destroyMesh();
  void releaseFont();
  [[nodiscard]] std::string fontCacheKey() const;

  std::string text_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  TextureHandle fontTexture_;
  PipelineHandle pipeline_;
  TextAlignment alignment_ = TextAlignment::CENTER;
  float layoutWidth_ = 0.0f;
  const float kerning_ = 2.0f;

  const float desiredPixelHeight_ = 18.0f;
  float fontScale_ = 1.0f;
  float fontSize_ = 0.0f;

  const uint32_t fontAtlasWidth_ = 1024;
  const uint32_t FontAtlasHeight_ = 1024;
  const uint32_t fontOversampleX_ = 2;
  const uint32_t fontOversampleY_ = 2;
  const uint8_t fontFirstChar_ = 32;
  const uint8_t fontCharCount_ = 255 - 32;
  std::shared_ptr<stbtt_packedchar[]> fontCharInfo_;
  std::string fontPath_;
};

} // namespace DL
