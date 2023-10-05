//
// Created by Mikael Deurell on 2023-08-16.
//

#pragma once

#include "camera.h"
#include "plane.h"
#include "scenenode.h"
#include "shader.h"
#include <string_view>

class PlaneNode : public DL::SceneNode {
public:
  explicit PlaneNode(std::string_view glslVersionString,
                     DL::SceneNode *parentNode = nullptr)
      : DL::SceneNode(parentNode),
        mGlslVersionString(glslVersionString.data()) {}

  ~PlaneNode() override = default;

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
