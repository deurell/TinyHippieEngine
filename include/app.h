#pragma once
#include "audiosystem.h"
#include "basisu_transcoder.h"
#include "game/deflektorish/deflektorishsound.h"
#include "game/deflektorish/deflektorishsoundmap.h"
#include "iscene.h"
#include "meshassetcache.h"
#include "renderqueue.h"
#include "renderresourcecache.h"
#include "scenelifecycle.h"
#include "scenemanager.h"
#include "renderdevice.h"
#include <array>
#include <GLFW/glfw3.h>
#include <map>
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
  void setTouchMoveAxis(glm::vec2 axis);
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
  bool bloomColorGradeEnabled() const;
  void setBloomColorGradeEnabled(bool enabled);
  float bloomIntensity() const;
  void setBloomIntensity(float intensity);
  float bloomThreshold() const;
  void setBloomThreshold(float threshold);
  float colorGradeSaturation() const;
  void setColorGradeSaturation(float saturation);
  float colorGradeContrast() const;
  void setColorGradeContrast(float contrast);
  float colorGradeWarmth() const;
  void setColorGradeWarmth(float warmth);
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
  float crtChromaticStrength() const;
  void setCrtChromaticStrength(float strength);
  float crtBrightness() const;
  void setCrtBrightness(float strength);
  bool renderCullingEnabled() const { return renderCullingEnabled_; }
  void setRenderCullingEnabled(bool enabled) { renderCullingEnabled_ = enabled; }
  RenderQueueStats lastRenderQueueStats() const { return lastRenderQueueStats_; }
  void submitDeflektorPostBump(glm::vec2 gamePosition, float strength);
  void submitDeflektorSound(Deflektorish::Sound sound,
                            glm::vec2 gamePosition, float energy);

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

  struct DeflektorPostBump {
    glm::vec2 uv{0.0f};
    float age = 0.0f;
    float strength = 0.0f;
  };

  bool init();
  void shutdown();
  void basisInit();
  void calculateDeltaTime();
  void initActionMap();
  void loadAudioClips();
  bool canPlayDeflektorSound(Deflektorish::Sound sound,
                             const Deflektorish::SoundEventConfig &event);
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
  void syncObservedWindowSizes();
  bool hasEnabledPostProcessEffects() const;
  PostProcessEffect *findPostProcessEffect(std::string_view name);
  const PostProcessEffect *findPostProcessEffect(std::string_view name) const;
  UniformValue *findEffectUniform(PostProcessEffect &effect,
                                  std::string_view name);
  const UniformValue *findEffectUniform(const PostProcessEffect &effect,
                                        std::string_view name) const;
  void updateDeflektorPostBumps(float dt);
  void syncDeflektorPostBumpUniforms();
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
  bool renderCullingEnabled_ = true;
  RenderQueueStats lastRenderQueueStats_;
  std::vector<DeflektorPostBump> deflektorPostBumps_;
  Deflektorish::SoundMapConfig deflektorSoundMap_;
  std::map<Deflektorish::Sound, std::vector<AudioSystem::SoundId>>
      deflektorActiveSounds_;
  ActionMap actionMap_;
  InputState inputState_;
  glm::vec2 touchMoveAxis_{0.0f};
  glm::vec2 lastMousePosition_{0.0f};
  glm::ivec2 lastObservedWindowSize_{0, 0};
  glm::ivec2 lastObservedFramebufferSize_{0, 0};

  bool hasLastMousePosition_ = false;
  glm::vec2 getWindowSize() const;
  glm::vec2 getFramebufferSize() const;
};
} // namespace DL
