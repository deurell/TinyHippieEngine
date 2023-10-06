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

  GLuint mVAO = 0;
  GLuint mVBO = 0;
  GLuint mUVBuffer = 0;
  GLuint mIndexBuffer = 0;
  uint16_t mIndexElementCount = 0;
  GLuint mFontTexture = 0;

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