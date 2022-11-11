//
// Created by Mikael Deurell on 2021-05-03.
//
#pragma once

#include "iscene.h"
#include "camera.h"
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
  void onClick(double x, double y) override {}
  void onKey(int key) override {}
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void renderLogo(float delta);

  std::string mGlslVersionString;

  std::unique_ptr<DL::Shader> mLogoShader;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::TextSprite> mLogoSprite;
  glm::vec2 mScreenSize{0, 0};

  std::unique_ptr<DL::Plane> mPlane;
  std::unique_ptr<DL::Plane> mPlane2;
  float mOffset = 0;
  float mTweak = 0.6;
};
