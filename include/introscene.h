//
// Created by Mikael Deurell on 2021-05-03.
//
#pragma once

#import "IScene.h"
#include "camera.h"
#include "shader.h"
#include "textsprite.h"
#import <string>
#import <string_view>

class IntroScene : public DL::IScene
{
public:
  explicit IntroScene(std::string_view glslVersionString);
  ~IntroScene() override = default;

  void init() override;
  void render(float delta) override;
  void onKey(int key) override {}
  void onScreenSizeChanged(glm::vec2 size) override {}

private:
  std::string mGlslVersionString;

  std::unique_ptr<DL::Shader> mLogoShader;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::TextSprite> mLogoSprite;
  glm::vec3 mLogoOffset {0,0,0};
};
