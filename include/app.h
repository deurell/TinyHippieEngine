#pragma once
#include "basisu_transcoder.h"
#include "camera.h"
#include "iscene.h"
#include "meshassetcache.h"
#include "renderresourcecache.h"
#include "scenelifecycle.h"
#include "scenemanager.h"
#include "model.h"
#include "renderdevice.h"
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
  void setFrameRateLimit(float fps);
  IScene *currentScene() const { return scene_.get(); }
  glm::vec2 windowSize() const { return getWindowSize(); }
  glm::vec2 framebufferSize() const { return getFramebufferSize(); }
  float fixedTimeStep() const { return fixedTimeStep_; }
  int lastFixedUpdateCount() const { return lastFixedUpdateCount_; }
  bool simulationPaused() const { return simulationPaused_; }
  void setSimulationPaused(bool paused) { simulationPaused_ = paused; }
  void requestSimulationStep() { ++requestedSimulationSteps_; }

  static constexpr char const *windows_title = "tiny hippie engine";
  static constexpr float screen_width = 1280;
  static constexpr float screen_height = 720;

private:
  bool init();
  void basisInit();
  void calculateDeltaTime();
  void loadSimpleScene();
  void loadCurrentScene();
  void registerScenes();

  GLFWwindow *window_{};
  std::unique_ptr<DL::IScene> scene_;
  SceneManager sceneManager_;
  std::unique_ptr<basist::etc1_global_selector_codebook> codebook_;
  std::unique_ptr<DL::IRenderDevice> renderDevice_;
  std::unique_ptr<DL::MeshAssetCache> meshAssetCache_;
  std::unique_ptr<DL::RenderResourceCache> renderResourceCache_;
  std::string glslVersionString_;
  float deltaTime_ = 0.0f;
  float startFrameTime_ = 0.0f;
  float lastFrameTime_ = 0.0f;
  float desiredFrameTime_ = 0.0f;
  float fixedTimeAccumulator_ = 0.0f;
  static constexpr float fixedTimeStep_ = 1.0f / 60.0f;
  int lastFixedUpdateCount_ = 0;
  int requestedSimulationSteps_ = 0;
  bool simulationPaused_ = false;
  bool nextSceneHeld_ = false;
  bool prevSceneHeld_ = false;
  glm::vec2 getWindowSize() const;
  glm::vec2 getFramebufferSize() const;
};
} // namespace DL
