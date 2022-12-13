#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stb_truetype.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace DL {

struct GlyphInfo {
  glm::vec3 positions[4];
  glm::vec2 uvs[4];
  float offsetX = 0;
  float offsetY = 0;
};

class TextSprite {

public:
  explicit TextSprite(std::string_view fontPath);
  TextSprite(std::string_view fontPath, std::string_view text);
  TextSprite(GLuint texture, stbtt_packedchar *fontInfo, std::string_view text);
  ~TextSprite() = default;

  void render(float delta) const;
  stbtt_packedchar *getFontCharInfoPtr();

  std::string mText;
  GLuint mVAO = 0;
  GLuint mVBO = 0;
  GLuint mUVBuffer = 0;
  GLuint mIndexBuffer = 0;
  uint16_t mIndexElementCount = 0;
  GLuint mFontTexture = 0;

private:
  void loadFontTexture(std::string_view fontPath);
  void init();
  GlyphInfo makeGlyphInfo(char character, float offsetX, float offsetY);

  const uint32_t mFontSize = 40;
  const uint32_t mFontAtlasWidth = 1024;
  const uint32_t mFontAtlasHeight = 1024;
  const uint32_t mFontOversampleX = 2;
  const uint32_t mFontOversampleY = 2;
  const uint8_t mFontFirstChar = 32;
  const uint8_t mFontCharCount = 255 - 32;
  std::unique_ptr<stbtt_packedchar[]> mFontCharInfo;
  stbtt_packedchar *mFontCharInfoPtr = nullptr;
};
} // namespace DL