#pragma once

#include "IScene.h"
#include "camera.h"
#include "shader.h"
#include "stb_truetype.h"
#include "texture.h"
#include "textsprite.h"
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
  void renderScroll(float delta);
  void renderStatus(float delta);

  std::unique_ptr<DL::Shader> mLabelShader;
  std::unique_ptr<DL::Camera> mLabelCamera;
  std::unique_ptr<DL::TextSprite> mTextSprite;

  std::unique_ptr<DL::Shader> mStatusShader;
  std::unique_ptr<DL::TextSprite> mStatusSprite;

  std::string mGlslVersionString;
  glm::vec2 mScreenSize;
  float mScrollOffset = 0;
  const float scroll_wrap = 60;
  float mDelta;
};
