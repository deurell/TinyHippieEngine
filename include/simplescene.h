//
// Created by Mikael Deurell on 2021-12-15.
//
#pragma once

#include "camera.h"
#include "iscene.h"
#include "plane.h"
#include "shader.h"
#include <string_view>

class SimpleScene : public DL::IScene {
public:
  explicit SimpleScene(std::string_view glslVersionString);
  ~SimpleScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override;
  void onKey(int key) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::Plane> mPlane;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;
};
