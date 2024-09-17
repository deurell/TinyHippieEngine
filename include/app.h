#pragma once
#include "basisu_transcoder.h"
#include "camera.h"
#include "iscene.h"
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
  void update();
  void render();
  void processInput(GLFWwindow *window);
  void onClick(int button, int action, int mod);
  void onKey(int key, int scancode, int action, int mod);
  void onScreenSizeChanged(int width, int height);
  void onFramebufferSizeChanged(int width, int height);

  static constexpr char const *windows_title = "tiny hippie engine";
  static constexpr float screen_width = 1024;
  static constexpr float screen_height = 768;

private:
  void init();
  void basisInit();
  glm::vec2 getScreenSize();
  void calculateDeltaTime();

  GLFWwindow *window_{};
  std::unique_ptr<DL::IScene> scene_;
  std::unique_ptr<basist::etc1_global_selector_codebook> codebook_;
  std::string glslVersionString_;
  float deltaTime_ = 0.0f;
  float startFrameTime_ = 0.0f;
  float lastFrameTime_ = 0.0f;
  float desiredFrameTime_ = 1.0f / 60.0f;
};
} // namespace DL
