//
// Created by Mikael Deurell on 2023-08-16.
//

#pragma once

#include "scenenode.h"
#include "camera.h"
#include "plane.h"
#include "shader.h"
#include <string_view>

class SimpleNodeScene : public DL::SceneNode {
public:
  explicit SimpleNodeScene(std::string_view glslVersionString);
  ~SimpleNodeScene() override = default;

  void init() override;
  void render(float delta) override;
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void initCamera();
  void initComponents();

  std::unique_ptr<DL::Camera> mCamera;
  glm::vec2 mScreenSize{0, 0};
  std::string mGlslVersionString;
};
