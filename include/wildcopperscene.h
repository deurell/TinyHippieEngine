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

  void render(float delta) {
    shader.use();

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::scale(model, glm::vec3(0.1, 0.1, 0.2));
    glm::vec3 polarCoord{amp * cos(angle), amp * sin(angle), 20.0f};
    model = glm::translate(model, polarCoord);
    model =
        glm::rotate(model, angle - (float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f));

    model = glm::translate(model, glm::vec3(0.0f, 16.0f, 0.0f));
    model = glm::rotate(model, flipDegree + angle, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, -16.0f, 0.0f));

    // model = scale * translate * rotate * glm::inverse(locTra) * rotLoc *
    // locTra;

    glm::mat4 view = camera.getViewMatrix();

    glm::mat4 projection =
        camera.getPerspectiveTransform(45.0, screenSize.x / screenSize.y);
    glm::mat4 mvp = projection * view * model;
    shader.setMat4f("mvp", mvp);

    shader.setFloat("deg", angle);
    shader.setFloat("iTime", static_cast<float>(glfwGetTime()));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, charSprite->mFontTexture);
    shader.setInt("texture1", 0);

    charSprite->render(delta);
  }

  glm::vec2 screenSize;
  float angle = 0;
  float amp = 200;
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
  void render(float delta) override;
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
