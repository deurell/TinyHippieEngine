//
// Created by Mikael Deurell on 2021-05-03.
//
#pragma once

#include "camera.h"
#include "iscene.h"
#include "plane.h"
#include "shader.h"
#include "textsprite.h"
#include <functional>
#include <string>
#include <string_view>

class IntroScene : public DL::IScene {
public:
  explicit IntroScene(std::string_view glslVersionString);
  ~IntroScene() override = default;

  void init() override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void renderLogoTop(float delta);
  void renderCodeLabel(float delta);
  void renderLogoBottom(float delta);

  std::string mGlslVersionString;

  std::unique_ptr<DL::Shader> mLogoShader;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::TextSprite> mLogoTop;
  std::unique_ptr<DL::TextSprite> mCodeLabel;
  std::unique_ptr<DL::TextSprite> mLogoBottom;
  glm::vec2 mScreenSize{0, 0};

  std::unique_ptr<DL::Plane> mPlane;
  std::unique_ptr<DL::Plane> mPlane2;
  float mOffset = 4;
  float mTweak = 8;
  float mTopOffset = -64;
  float mMiddleOffset = -80;
  float mBottomOffset = -36;
};
