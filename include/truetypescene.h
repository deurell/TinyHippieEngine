#pragma once

#include "IScene.h"
#include "camera.h"
#include "shader.h"
#include "stb_truetype.h"
#include "texture.h"
#include <memory>

class TrueTypeScene : public DL::IScene {
public:
  TrueTypeScene(std::string glslVersion);
  ~TrueTypeScene() = default;

  virtual void init() override;
  virtual void render(float delta) override;
  virtual void onKey(int key) override;
  virtual void onScreenSizeChanged(glm::vec2 size) override;

private:
  void initFont();

  std::unique_ptr<DL::Shader> mShader;
  unsigned int mVAO, mVBO, mEBO;
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
};