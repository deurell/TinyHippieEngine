#pragma once
#include "audiosystem.h"
#include "basisu_transcoder.h"
#include "iscene.h"
#include "meshassetcache.h"
#include "renderresourcecache.h"
#include "scenelifecycle.h"
#include "scenemanager.h"
#include "renderdevice.h"
#include <array>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace DL {
class App {
public:
  App() = default;
  ~App();

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
  AudioSystem &audioSystem() { return audioSystem_; }
  const AudioSystem &audioSystem() const { return audioSystem_; }
  bool postProcessEnabled() const;
  void setPostProcessEnabled(bool enabled);
  bool chromaticEnabled() const;
  void setChromaticEnabled(bool enabled);
  float chromaticStrength() const;
  void setChromaticStrength(float strength);
  bool crtEnabled() const;
  void setCrtEnabled(bool enabled);
  float crtScanlineStrength() const;
  void setCrtScanlineStrength(float strength);
  float crtVignetteStrength() const;
  void setCrtVignetteStrength(float strength);
  float crtCurvature() const;
  void setCrtCurvature(float strength);
  float crtWobble() const;
  void setCrtWobble(float strength);
  float crtGrilleStrength() const;
  void setCrtGrilleStrength(float strength);
  float crtBrightness() const;
  void setCrtBrightness(float strength);

  static constexpr char const *windows_title = "tiny hippie engine";
  static constexpr float screen_width = 1280;
  static constexpr float screen_height = 720;

private:
  struct PostProcessEffect {
    std::string name;
    bool enabled = true;
    std::string vertexShaderPath = "Shaders/postprocess.vert";
    std::string fragmentShaderPath;
    PipelineHandle pipeline;
    std::vector<UniformValue> uniforms;
  };

  struct PostProcessStack {
    bool enabled = true;
    std::vector<PostProcessEffect> effects;
  };

  bool init();
  void shutdown();
  void basisInit();
  void calculateDeltaTime();
  void initActionMap();
  void loadCurrentScene();
  void registerScenes();
  void configureDefaultPostProcessStack();
  void ensurePostProcessResources(std::uint32_t framebufferWidth,
                                  std::uint32_t framebufferHeight);
  void renderScenePass(const FrameContext &ctx,
                       std::uint32_t framebufferWidth,
                       std::uint32_t framebufferHeight);
  void renderPostProcessPass(const FrameContext &ctx,
                             std::uint32_t framebufferWidth,
                             std::uint32_t framebufferHeight);
  bool hasEnabledPostProcessEffects() const;
  PostProcessEffect *findPostProcessEffect(std::string_view name);
  const PostProcessEffect *findPostProcessEffect(std::string_view name) const;
  UniformValue *findEffectUniform(PostProcessEffect &effect,
                                  std::string_view name);
  const UniformValue *findEffectUniform(const PostProcessEffect &effect,
                                        std::string_view name) const;
  glm::vec2 mapMousePositionToScene(glm::vec2 mousePosition,
                                    glm::vec2 windowSize) const;

  GLFWwindow *window_{};
  AudioSystem audioSystem_;
  std::unique_ptr<DL::IScene> scene_;
  SceneManager sceneManager_;
  std::unique_ptr<basist::etc1_global_selector_codebook> codebook_;
  std::unique_ptr<DL::IRenderDevice> renderDevice_;
  std::unique_ptr<DL::MeshAssetCache> meshAssetCache_;
  std::unique_ptr<DL::RenderResourceCache> renderResourceCache_;
  RenderTargetHandle sceneRenderTarget_;
  MeshHandle postProcessQuad_;
  std::array<RenderTargetHandle, 2> postProcessTargets_{};
  glm::uvec2 postProcessTargetSize_{0u, 0u};
  PostProcessStack postProcessStack_;
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
  ActionMap actionMap_;
  InputState inputState_;
  glm::vec2 lastMousePosition_{0.0f};

  bool hasLastMousePosition_ = false;
  glm::vec2 getWindowSize() const;
  glm::vec2 getFramebufferSize() const;
};
} // namespace DL
