#pragma once

#include "IScene.h"
#include "camera.h"
#include "shader.h"
#include "stb_truetype.h"
#include "texture.h"
#include <memory>

struct GlyphInfo {
  glm::vec3 positions[4];
  glm::vec2 uvs[4];
  float offsetX = 0;
  float offsetY = 0;
};

class TrueTypeScene : public DL::IScene {
public:
  TrueTypeScene(std::string glslVersion);
  ~TrueTypeScene() = default;

  virtual void init() override;
  virtual void render(float delta) override;
  virtual void onKey(int key) override;
  virtual void onScreenSizeChanged(glm::vec2 size) override;

private:
  void loadFontTexture();
  void initLabel();
  void renderLabel(float delta);
  GlyphInfo makeGlyphInfo(uint32_t character, float offsetX, float offsetY);
  std::unique_ptr<DL::Shader> mLabelShader;
  std::unique_ptr<DL::Camera> mLabelCamera;
  std::string mGlslVersionString;
  glm::vec2 mScreenSize;
  float mDelta;

  struct {
    const uint32_t size = 40;
    const uint32_t atlasWidth = 1024;
    const uint32_t atlasHeight = 1024;
    const uint32_t oversampleX = 2;
    const uint32_t oversampleY = 2;
    const uint32_t firstChar = ' ';
    const uint32_t charCount = '~' - ' ';
    std::unique_ptr<stbtt_packedchar[]> charInfo;
    GLuint texture = 0;
  } font;

  struct {
    GLuint vao = 0;
    GLuint vertexBuffer = 0;
    GLuint uvBuffer = 0;
    GLuint indexBuffer = 0;
    uint16_t indexElementCount = 0;
    float angle = 0;
  } rotatingLabel;
};