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
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
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
  float mOffset = 0.34;
  float mTweak = -3.1;
  float mLogoOffset = -127;
  int mBars = 8;
  glm::vec3 mBaseCol = {0.47, 0.57, 0.78};
};
