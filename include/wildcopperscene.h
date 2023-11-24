#pragma once

#include "camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/matrix.hpp"
#include "iscene.h"
#include "plane.h"
#include "shader.h"
#include "textsprite.h"
#include <cmath>
#include <string>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include "math.h"
#endif

struct ScrollChar {
public:
  ScrollChar(DL::Camera &camera, DL::Shader &shader, glm::vec2 screenSize,
             GLuint texture, stbtt_packedchar *fontInfo, std::string text)
      : shader(shader), camera(camera), screenSize(screenSize) {
    charSprite = std::make_unique<DL::TextSprite>(texture, fontInfo, text);
  }

  void render(float delta);

  glm::vec2 screenSize;
  float angle = 0;
  float amp = 172;
  float flipDegree = 0;
  std::unique_ptr<DL::TextSprite> charSprite;
  DL::Shader &shader;
  DL::Camera &camera;
};

class WildCopperScene : public DL::IScene {
public:
  explicit WildCopperScene(std::string_view glslVersionString);
  ~WildCopperScene() override = default;

  void init() override;
  void update(float delta) override;
  void render(float delta) override;
  void onClick(double x, double y) override {}
  void onKey(int key) override {}
  void onScreenSizeChanged(glm::vec2 size) override;

private:
  void renderScroller(float delta);
  void wrap();

  std::string mGlslVersionString;

  std::unique_ptr<DL::Shader> mShader;
  std::unique_ptr<DL::Camera> mCamera;
  std::unique_ptr<DL::TextSprite> mTextSprite;
  glm::vec2 mScreenSize{0, 0};

  std::vector<std::unique_ptr<ScrollChar>> mScrollChars;
  std::unique_ptr<DL::Plane> mPlane;
};
