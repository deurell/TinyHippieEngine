#pragma once

#include "camera.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "shader.h"
#include "textsprite.h"
#include "visualizerbase.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace DL {
class TextVisualizer : public VisualizerBase {
public:
  explicit TextVisualizer(std::string name, DL::Camera &camera,
                          std::string_view glslVersionString, SceneNode &node,
                          std::string_view text);

  void render(const glm::mat4 &worldTransform, float delta) override;

private:
  void loadFontTexture(std::string_view fontPath);
  stbtt_packedchar *getFontCharInfoPtr();
  GlyphInfo makeGlyphInfo(char character, float offsetX, float offsetY);
  void initGraphics();

  std::string_view text_;
  std::unique_ptr<DL::Shader> shader_ = nullptr;

  GLuint VAO_ = 0;
  GLuint VBO_ = 0;
  GLuint UVBuffer_ = 0;
  GLuint indexBuffer_ = 0;
  uint16_t indexElementCount_ = 0;
  GLuint fontTexture_ = 0;

  const float desiredPixelHeight_ = 24.0f;
  float fontScale_;
  float fontSize_ = 0.0f;

  const uint32_t fontAtlasWidth_ = 1024;
  const uint32_t FontAtlasHeight_ = 1024;
  const uint32_t fontOversampleX_ = 2;
  const uint32_t fontOversampleY_ = 2;
  const uint8_t fontFirstChar_ = 32;
  const uint8_t fontCharCount_ = 255 - 32;
  std::unique_ptr<stbtt_packedchar[]> fontCharInfo_;
  stbtt_packedchar *fontCharInfoPtr_ = nullptr;
};

} // namespace DL