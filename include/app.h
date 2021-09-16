#pragma once
#include "iscene.h"
#include "basisu_transcoder.h"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include <GLFW/glfw3.h>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace DL {
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

  static constexpr char const *windows_title = "tiny hippie engine";
  static constexpr float screen_width = 1024;
  static constexpr float screen_height = 768;
  GLFWwindow *mWindow;
  std::unique_ptr<DL::IScene> mScene;
  std::unique_ptr<basist::etc1_global_selector_codebook> mCodebook;
  std::string mGlslVersionString;
  float mDeltaTime = 0.0f;
  float mLastFrame = 0.0f;
};
} // namespace DL
