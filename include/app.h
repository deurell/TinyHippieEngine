#pragma once

#include "camera.h"
#include "model.h"
#include "shader.h"
#include <GLFW/glfw3.h>

#ifdef Emscripten
#include <emscripten.h>
#endif

#include <memory>

class App {
public:
  App() = default;
  ~App() = default;

  int run();
  void render();
  void processInput(GLFWwindow *window);

private:
  void init();

  void basisInit();

  GLFWwindow *mWindow;
  // std::unique_ptr<Texture> mTexture;
  std::unique_ptr<Shader> mLightingShader;
  std::unique_ptr<Shader> mLampShader;
  std::unique_ptr<Camera> mCamera;
  std::unique_ptr<Model> mModel;
  // std::unique_ptr<basist::etc1_global_selector_codebook> mCodebook;

  unsigned int mLightVAO;
  unsigned int mCubeVAO;

  static constexpr float screen_width = 1024;
  static constexpr float screen_height = 768;

  glm::vec3 mLightPos = {1.2f, 1.0f, 2.0f};

  static constexpr int point_light_count = 2;
  glm::vec3 mPointLightPositions[point_light_count] = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f)};

  float mDeltaTime = 0.0f;
  float mLastFrame = 0.0f;

  float mXDeg = 0.0f;
  float mYDeg = 0.0f;

  glm::vec3 mModelTranslate = {0.0f, 0.0f, 0.0f};
  glm::vec3 mObjectColor = {0.3, 0.8, 0.3};
};