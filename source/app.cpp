#include "app.h"
#include "GLFW/glfw3.h"
#include "debugui.h"
#include "glad/glad.h"
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif
#include "game/scenes/skeletalanimationblendscene.h"
#include "game/scenes/inputdebugscene.h"
#include "game/scenes/textstarterscene.h"
#include "logger.h"
#include "renderqueue.h"
#include "scenemanager.h"
#ifdef TINY_ENGINE_ENABLE_PHYSICS
#include "game/scenes/physicstestscene.h"
#endif
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>

namespace {
constexpr char kCrtEffectName[] = "CRT";
constexpr char kCrtCurvatureUniform[] = "crtCurvature";
constexpr float kCrtCurveScale = 0.94f;
constexpr float kCrtCurveOffset = 0.03f;
DL::App *gActiveApp = nullptr;

glm::vec2 applyCrtCurve(glm::vec2 uv, float curvature, glm::vec2 screenSize) {
  // Keep in sync with Shaders/crt.frag curve().
  const float c = std::clamp(curvature, 0.0f, 1.0f);
  if (c <= 0.0f) {
    return uv;
  }

  const float aspect = screenSize.x / std::max(screenSize.y, 1.0f);
  glm::vec2 p = (uv - glm::vec2(0.5f)) * 2.0f;
  p.x *= aspect;

  glm::vec2 curved = p;
  curved.x *= 1.0f + std::pow(std::abs(p.y), 2.0f) * 0.045f;
  curved.y *= 1.0f + std::pow(std::abs(p.x), 2.0f) * 0.045f;
  curved.x /= aspect;
  curved = curved * 0.5f + glm::vec2(0.5f);
  curved = curved * kCrtCurveScale + glm::vec2(kCrtCurveOffset);
  return glm::mix(uv, curved, c);
}
} // namespace

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void tiny_set_touch_move_axis(float x,
                                                              float y) {
  if (gActiveApp == nullptr) {
    return;
  }
  gActiveApp->setTouchMoveAxis({x, y});
}
#endif

void renderloop_callback(void *arg) {
  auto app = static_cast<DL::App *>(arg);
  app->update();
  app->render();
}

void mouseclick_callback(GLFWwindow *window, int button, int action, int mod) {
#ifdef USE_IMGUI
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mod);
#endif
  if (auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window))) {
    app->onClick(button, action, mod);
  }
}

void cursorpos_callback(GLFWwindow *window, double x, double y) {
#ifdef USE_IMGUI
  ImGui_ImplGlfw_CursorPosCallback(window, x, y);
#endif
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
#ifdef USE_IMGUI
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
#endif
}

void keyclick_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mods) {
#ifdef USE_IMGUI
  ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
#endif
  if (auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window))) {
    app->onKey(key, scancode, action, mods);
  }
}

void char_callback(GLFWwindow *window, unsigned int c) {
#ifdef USE_IMGUI
  ImGui_ImplGlfw_CharCallback(window, c);
#endif
}

void window_size_callback(GLFWwindow *window, int width, int height) {
  auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window));
  app->onScreenSizeChanged(width, height);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window));
  app->onFramebufferSizeChanged(width, height);
}

DL::App::~App() { shutdown(); }

bool DL::App::postProcessEnabled() const { return postProcessStack_.enabled; }

void DL::App::setPostProcessEnabled(bool enabled) {
  postProcessStack_.enabled = enabled;
}

bool DL::App::chromaticEnabled() const {
  const auto *effect = findPostProcessEffect("Chromatic Aberration");
  return effect != nullptr && effect->enabled;
}

void DL::App::setChromaticEnabled(bool enabled) {
  if (auto *effect = findPostProcessEffect("Chromatic Aberration")) {
    effect->enabled = enabled;
  }
}

float DL::App::chromaticStrength() const {
  const auto *effect = findPostProcessEffect("Chromatic Aberration");
  const auto *uniform =
      effect != nullptr
          ? findEffectUniform(*effect, "chromaticAberrationStrength")
          : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setChromaticStrength(float strength) {
  if (auto *effect = findPostProcessEffect("Chromatic Aberration")) {
    if (auto *uniform =
            findEffectUniform(*effect, "chromaticAberrationStrength")) {
      uniform->float_value = strength;
    }
  }
}

bool DL::App::bloomColorGradeEnabled() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  return effect != nullptr && effect->enabled;
}

void DL::App::setBloomColorGradeEnabled(bool enabled) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    effect->enabled = enabled;
  }
}

float DL::App::bloomIntensity() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "bloomIntensity")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setBloomIntensity(float intensity) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    if (auto *uniform = findEffectUniform(*effect, "bloomIntensity")) {
      uniform->float_value = intensity;
    }
  }
}

float DL::App::bloomThreshold() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "bloomThreshold")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 1.0f;
}

void DL::App::setBloomThreshold(float threshold) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    if (auto *uniform = findEffectUniform(*effect, "bloomThreshold")) {
      uniform->float_value = threshold;
    }
  }
}

float DL::App::colorGradeSaturation() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "colorGradeSaturation")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 1.0f;
}

void DL::App::setColorGradeSaturation(float saturation) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    if (auto *uniform = findEffectUniform(*effect, "colorGradeSaturation")) {
      uniform->float_value = saturation;
    }
  }
}

float DL::App::colorGradeContrast() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "colorGradeContrast")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 1.0f;
}

void DL::App::setColorGradeContrast(float contrast) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    if (auto *uniform = findEffectUniform(*effect, "colorGradeContrast")) {
      uniform->float_value = contrast;
    }
  }
}

float DL::App::colorGradeWarmth() const {
  const auto *effect = findPostProcessEffect("Bloom Color Grade");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "colorGradeWarmth")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setColorGradeWarmth(float warmth) {
  if (auto *effect = findPostProcessEffect("Bloom Color Grade")) {
    if (auto *uniform = findEffectUniform(*effect, "colorGradeWarmth")) {
      uniform->float_value = warmth;
    }
  }
}

bool DL::App::crtEnabled() const {
  const auto *effect = findPostProcessEffect("CRT");
  return effect != nullptr && effect->enabled;
}

void DL::App::setCrtEnabled(bool enabled) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    effect->enabled = enabled;
  }
}

float DL::App::crtScanlineStrength() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform = effect != nullptr
                            ? findEffectUniform(*effect, "crtScanlineStrength")
                            : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtScanlineStrength(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtScanlineStrength")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtVignetteStrength() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform = effect != nullptr
                            ? findEffectUniform(*effect, "crtVignetteStrength")
                            : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtVignetteStrength(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtVignetteStrength")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtCurvature() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "crtCurvature") : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtCurvature(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtCurvature")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtWobble() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform = effect != nullptr
                            ? findEffectUniform(*effect, "crtWobbleStrength")
                            : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtWobble(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtWobbleStrength")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtGrilleStrength() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform = effect != nullptr
                            ? findEffectUniform(*effect, "crtGrilleStrength")
                            : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtGrilleStrength(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtGrilleStrength")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtChromaticStrength() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "crtChromaticStrength")
                        : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtChromaticStrength(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtChromaticStrength")) {
      uniform->float_value = strength;
    }
  }
}

float DL::App::crtBrightness() const {
  const auto *effect = findPostProcessEffect("CRT");
  const auto *uniform =
      effect != nullptr ? findEffectUniform(*effect, "crtBrightness") : nullptr;
  return uniform != nullptr ? uniform->float_value : 0.0f;
}

void DL::App::setCrtBrightness(float strength) {
  if (auto *effect = findPostProcessEffect("CRT")) {
    if (auto *uniform = findEffectUniform(*effect, "crtBrightness")) {
      uniform->float_value = strength;
    }
  }
}

bool DL::App::init() {
  if (!glfwInit()) {
    LogError("GLFW initialization failed");
    return false;
  }
  initActionMap();
  basisInit();
  configureDefaultPostProcessStack();
#ifdef __EMSCRIPTEN__
  glslVersionString_ = "#version 300 es\n";
#else
  glslVersionString_ = "#version 330 core\n";
#endif

  registerScenes();
  if (!sceneManager_.hasScenes()) {
    LogError("No scenes registered");
    return false;
  }
  return true;
}

void DL::App::initActionMap() {
  actionMap_.bind(Action::MoveForward, Key::W);
  actionMap_.bind(Action::MoveBackward, Key::S);
  actionMap_.bind(Action::MoveLeft, Key::A);
  actionMap_.bind(Action::MoveRight, Key::D);
  actionMap_.bind(Action::Fire, Key::Space);
}

int DL::App::run() {
  if (!init()) {
    shutdown();
    return EXIT_FAILURE;
  }
  gActiveApp = this;

#ifdef __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  window_ = glfwCreateWindow(screen_width, screen_height, windows_title,
                             nullptr, nullptr);
  if (window_ == nullptr) {
    std::cerr << "window create failed" << std::endl;
    shutdown();
    return EXIT_FAILURE;
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetMouseButtonCallback(window_, mouseclick_callback);
  glfwSetCursorPosCallback(window_, cursorpos_callback);
  glfwSetScrollCallback(window_, scroll_callback);
  glfwSetKeyCallback(window_, keyclick_callback);
  glfwSetCharCallback(window_, char_callback);
  glfwSetWindowSizeCallback(window_, window_size_callback);
  glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);
  startFrameTime_ = lastFrameTime_ = static_cast<float>(glfwGetTime());

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    LogError("GLAD initialization failed");
    shutdown();
    return EXIT_FAILURE;
  }

  renderDevice_ = createOpenGLRenderDevice(glslVersionString_);
  meshAssetCache_ = std::make_unique<DL::MeshAssetCache>();
  renderResourceCache_ =
      std::make_unique<DL::RenderResourceCache>(*renderDevice_);

  if (!audioSystem_.init()) {
    LogWarn("Audio system initialization failed");
  }

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(window_, &frameWidth, &frameHeight);
  renderDevice_->setViewport(static_cast<std::uint32_t>(frameWidth),
                             static_cast<std::uint32_t>(frameHeight));

#ifdef USE_IMGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  applyDebugUiStyle();
  ImGui_ImplGlfw_InitForOpenGL(window_, false);
  ImGui_ImplOpenGL3_Init(glslVersionString_.c_str());
#endif

  loadCurrentScene();
#ifdef __APPLE__
  setFrameRateLimit(60.0f);
#endif

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(&renderloop_callback, this, -1, 1);
#else
  while (!glfwWindowShouldClose(window_)) {
    update();
    render();
  }
#endif
  shutdown();
  return EXIT_SUCCESS;
}

void DL::App::shutdown() {
  if (gActiveApp == this) {
    gActiveApp = nullptr;
  }
  scene_.reset();
  renderResourceCache_.reset();
  meshAssetCache_.reset();
  if (renderDevice_ != nullptr && sceneRenderTarget_.valid()) {
    renderDevice_->destroy(sceneRenderTarget_);
    sceneRenderTarget_ = {};
  }
  if (renderDevice_ != nullptr) {
    for (auto &target : postProcessTargets_) {
      if (target.valid()) {
        renderDevice_->destroy(target);
        target = {};
      }
    }
  }
  renderDevice_.reset();
  codebook_.reset();

  if (window_ != nullptr) {
#ifdef USE_IMGUI
    if (ImGui::GetCurrentContext() != nullptr) {
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
    }
#endif
    glfwSetWindowUserPointer(window_, nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  audioSystem_.shutdown();
  glfwTerminate();
}

void DL::App::update() {
  calculateDeltaTime();
  audioSystem_.update();
  if (window_) {
    processInput(window_);
    syncObservedWindowSizes();
  }
  if (!scene_)
    return;
  const auto frameTime = glfwGetTime();
  const glm::vec2 windowSize = getWindowSize();
  const glm::vec2 framebufferSize = getFramebufferSize();
  lastFixedUpdateCount_ = 0;
  if (simulationPaused_) {
    while (requestedSimulationSteps_ > 0) {
      scene_->fixedUpdate({fixedTimeStep_, frameTime, inputState_, windowSize,
                           framebufferSize});
      --requestedSimulationSteps_;
      ++lastFixedUpdateCount_;
    }
  } else {
    fixedTimeAccumulator_ += deltaTime_;
    while (fixedTimeAccumulator_ >= fixedTimeStep_) {
      scene_->fixedUpdate({fixedTimeStep_, frameTime, inputState_, windowSize,
                           framebufferSize});
      fixedTimeAccumulator_ -= fixedTimeStep_;
      ++lastFixedUpdateCount_;
    }
  }
  scene_->update(
      {deltaTime_, frameTime, inputState_, windowSize, framebufferSize});
}

void DL::App::render() {
  if (!scene_ || renderDevice_ == nullptr)
    return;
  int frameWidth = 0;
  int frameHeight = 0;
  glfwGetFramebufferSize(window_, &frameWidth, &frameHeight);
  const FrameContext frameCtx{
      deltaTime_,
      glfwGetTime(),
      inputState_,
      getWindowSize(),
      {static_cast<float>(frameWidth), static_cast<float>(frameHeight)}};
  ensurePostProcessResources(static_cast<std::uint32_t>(frameWidth),
                             static_cast<std::uint32_t>(frameHeight));

  DL::beginDebugUiFrame();
  renderScenePass(frameCtx, static_cast<std::uint32_t>(frameWidth),
                  static_cast<std::uint32_t>(frameHeight));
  renderPostProcessPass(frameCtx, static_cast<std::uint32_t>(frameWidth),
                        static_cast<std::uint32_t>(frameHeight));
  renderDevice_->endFrame();
  DL::drawEngineDebugWindows(*this, deltaTime_,
                             renderDevice_->getRenderStats());
  DL::drawLogWindow();

#ifdef USE_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  DL::endDebugUiFrame();
#endif

  renderDevice_->setViewport(static_cast<std::uint32_t>(frameWidth),
                             static_cast<std::uint32_t>(frameHeight));
  glfwSwapBuffers(window_);
  glfwPollEvents();

  auto endFrameTime = static_cast<float>(glfwGetTime());
  float frameTime = endFrameTime - startFrameTime_;

  if (desiredFrameTime_ > 0.0f) {
    float sleepTime = desiredFrameTime_ - frameTime;
    if (sleepTime > 0.0f) {
      std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
    }
  }
}

void DL::App::processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

#ifdef USE_IMGUI
  const bool imguiWantsMouse =
      ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
#else
  const bool imguiWantsMouse = false;
#endif

  inputState_.keysDown[static_cast<std::size_t>(Key::W)] =
      glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
  inputState_.keysDown[static_cast<std::size_t>(Key::A)] =
      glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
  inputState_.keysDown[static_cast<std::size_t>(Key::S)] =
      glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
  inputState_.keysDown[static_cast<std::size_t>(Key::D)] =
      glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
  inputState_.keysDown[static_cast<std::size_t>(Key::Space)] =
      glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  const glm::vec2 keyboardMoveAxis{
      (inputState_.isKeyDown(Key::D) ? 1.0f : 0.0f) -
          (inputState_.isKeyDown(Key::A) ? 1.0f : 0.0f),
      (inputState_.isKeyDown(Key::W) ? 1.0f : 0.0f) -
          (inputState_.isKeyDown(Key::S) ? 1.0f : 0.0f)};
  inputState_.moveAxis = keyboardMoveAxis + touchMoveAxis_;
  if (glm::length(inputState_.moveAxis) > 1.0f) {
    inputState_.moveAxis = glm::normalize(inputState_.moveAxis);
  }
  inputState_.mouseButtonsDown[static_cast<std::size_t>(MouseButton::Left)] =
      !imguiWantsMouse &&
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  inputState_.mouseButtonsDown[static_cast<std::size_t>(MouseButton::Right)] =
      !imguiWantsMouse &&
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  inputState_.mouseButtonsDown[static_cast<std::size_t>(MouseButton::Middle)] =
      !imguiWantsMouse &&
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  const glm::vec2 mousePosition{static_cast<float>(mouseX),
                                static_cast<float>(mouseY)};
  inputState_.mousePosition = mousePosition;
  inputState_.sceneMousePosition =
      mapMousePositionToScene(mousePosition, getWindowSize());
  inputState_.mouseDelta = hasLastMousePosition_
                               ? mousePosition - lastMousePosition_
                               : glm::vec2(0.0f);
  lastMousePosition_ = mousePosition;
  hasLastMousePosition_ = true;
  actionMap_.apply(inputState_, inputState_);
}

void DL::App::setTouchMoveAxis(glm::vec2 axis) {
  if (glm::length(axis) > 1.0f) {
    axis = glm::normalize(axis);
  }
  touchMoveAxis_ = axis;
}

glm::vec2 DL::App::mapMousePositionToScene(glm::vec2 mousePosition,
                                           glm::vec2 windowSize) const {
  if (!postProcessStack_.enabled || windowSize.x <= 0.0f ||
      windowSize.y <= 0.0f) {
    return mousePosition;
  }

  const auto *crtEffect = findPostProcessEffect(kCrtEffectName);
  if (crtEffect == nullptr || !crtEffect->enabled) {
    return mousePosition;
  }

  const auto *curvatureUniform =
      findEffectUniform(*crtEffect, kCrtCurvatureUniform);
  const float curvature =
      curvatureUniform != nullptr ? curvatureUniform->float_value : 1.0f;

  return applyCrtCurve(mousePosition / windowSize, curvature, windowSize) *
         windowSize;
}

void DL::App::loadCurrentScene() {
  auto previousScene = std::move(scene_);
  scene_ = replacePreparedScene(std::move(previousScene),
                                sceneManager_.createCurrent(), getWindowSize(),
                                getFramebufferSize());
  if (!scene_) {
    LogError("Failed to create scene");
  } else {
    Logger::instance().logEvent(LogLevel::Info, "scene", "scene_loaded");
  }
}

void DL::App::registerScenes() {
  sceneManager_.registerScene([this] {
    return std::make_unique<TextStarterScene>(
        renderDevice_.get(), codebook_.get(), meshAssetCache_.get(),
        renderResourceCache_.get());
  });
  sceneManager_.registerScene([this] {
    return std::make_unique<InputDebugScene>(renderDevice_.get(),
                                             renderResourceCache_.get());
  });
  sceneManager_.registerScene([this] {
    return std::make_unique<TextStarterScene>(
        renderDevice_.get(), codebook_.get(), meshAssetCache_.get(),
        renderResourceCache_.get(),
        "Resources/Scenes/tiny_dungeon_atlas.scene.json");
  });
  sceneManager_.registerScene([this] {
    return std::make_unique<TextStarterScene>(
        renderDevice_.get(), codebook_.get(), meshAssetCache_.get(),
        renderResourceCache_.get(),
        "Resources/Scenes/kenney_platformer.scene.json");
  });
#ifdef TINY_ENGINE_ENABLE_PHYSICS
  sceneManager_.registerScene([this] {
    return std::make_unique<PhysicsTestScene>(renderDevice_.get());
  });
#endif
  sceneManager_.registerScene([this] {
    return std::make_unique<SkeletalAnimationBlendScene>(
        renderDevice_.get(), codebook_.get(), meshAssetCache_.get(),
        renderResourceCache_.get());
  });
}

void DL::App::configureDefaultPostProcessStack() {
  if (!postProcessStack_.effects.empty()) {
    return;
  }

  PostProcessEffect bloomColorGradeEffect;
  bloomColorGradeEffect.name = "Bloom Color Grade";
  bloomColorGradeEffect.fragmentShaderPath = "Shaders/bloom_colorgrade.frag";
  bloomColorGradeEffect.uniforms.push_back(
      UniformValue::makeFloat("bloomIntensity", 0.08f));
  bloomColorGradeEffect.uniforms.push_back(
      UniformValue::makeFloat("bloomThreshold", 0.82f));
  bloomColorGradeEffect.uniforms.push_back(
      UniformValue::makeFloat("colorGradeSaturation", 1.04f));
  bloomColorGradeEffect.uniforms.push_back(
      UniformValue::makeFloat("colorGradeContrast", 1.02f));
  bloomColorGradeEffect.uniforms.push_back(
      UniformValue::makeFloat("colorGradeWarmth", 0.015f));
  bloomColorGradeEffect.enabled = false;
  postProcessStack_.effects.push_back(std::move(bloomColorGradeEffect));

  PostProcessEffect chromaticEffect;
  chromaticEffect.name = "Chromatic Aberration";
  chromaticEffect.fragmentShaderPath = "Shaders/chromatic_aberration.frag";
  chromaticEffect.uniforms.push_back(
      UniformValue::makeFloat("chromaticAberrationStrength", 0.0235f));
  chromaticEffect.enabled = false;
  postProcessStack_.effects.push_back(std::move(chromaticEffect));

  PostProcessEffect crtEffect;
  crtEffect.name = "CRT";
  crtEffect.fragmentShaderPath = "Shaders/crt.frag";
#ifdef __EMSCRIPTEN__
  constexpr float kDefaultCrtScanlineStrength = 0.450f;
  constexpr float kDefaultCrtVignetteStrength = 0.114f;
  constexpr float kDefaultCrtCurvature = 0.250f;
  constexpr float kDefaultCrtWobbleStrength = 0.0002f;
  constexpr float kDefaultCrtGrilleStrength = 0.135f;
  constexpr float kDefaultCrtChromaticStrength = 0.156f;
  constexpr float kDefaultCrtBrightness = 1.03f;
#else
  constexpr float kDefaultCrtScanlineStrength = 0.312f;
  constexpr float kDefaultCrtVignetteStrength = 0.040f;
  constexpr float kDefaultCrtCurvature = 0.316f;
  constexpr float kDefaultCrtWobbleStrength = 0.0003f;
  constexpr float kDefaultCrtGrilleStrength = 0.096f;
  constexpr float kDefaultCrtChromaticStrength = 0.221f;
  constexpr float kDefaultCrtBrightness = 1.05f;
#endif
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtScanlineStrength",
                              kDefaultCrtScanlineStrength));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtVignetteStrength",
                              kDefaultCrtVignetteStrength));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtCurvature", kDefaultCrtCurvature));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtWobbleStrength",
                              kDefaultCrtWobbleStrength));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtGrilleStrength",
                              kDefaultCrtGrilleStrength));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtChromaticStrength",
                              kDefaultCrtChromaticStrength));
  crtEffect.uniforms.push_back(
      UniformValue::makeFloat("crtBrightness", kDefaultCrtBrightness));
  postProcessStack_.effects.push_back(std::move(crtEffect));
}

void DL::App::ensurePostProcessResources(std::uint32_t framebufferWidth,
                                         std::uint32_t framebufferHeight) {
  if (renderDevice_ == nullptr || renderResourceCache_ == nullptr ||
      framebufferWidth == 0 || framebufferHeight == 0) {
    return;
  }

  configureDefaultPostProcessStack();
  if (!postProcessQuad_.valid()) {
    postProcessQuad_ = renderResourceCache_->acquireTexturedQuad();
  }
  for (auto &effect : postProcessStack_.effects) {
    if (!effect.pipeline.valid()) {
      effect.pipeline = renderResourceCache_->acquirePipeline(
          effect.vertexShaderPath, effect.fragmentShaderPath);
    }
  }

  if (!sceneRenderTarget_.valid()) {
    sceneRenderTarget_ =
        renderDevice_->createRenderTarget(framebufferWidth, framebufferHeight);
  } else if (postProcessTargetSize_.x != framebufferWidth ||
             postProcessTargetSize_.y != framebufferHeight) {
    renderDevice_->resizeRenderTarget(sceneRenderTarget_, framebufferWidth,
                                      framebufferHeight);
  }

  for (auto &target : postProcessTargets_) {
    if (!target.valid()) {
      target = renderDevice_->createRenderTarget(framebufferWidth,
                                                 framebufferHeight);
    } else if (postProcessTargetSize_.x != framebufferWidth ||
               postProcessTargetSize_.y != framebufferHeight) {
      renderDevice_->resizeRenderTarget(target, framebufferWidth,
                                        framebufferHeight);
    }
  }

  if (sceneRenderTarget_.valid() && postProcessTargets_[0].valid() &&
      postProcessTargets_[1].valid()) {
    postProcessTargetSize_ = {framebufferWidth, framebufferHeight};
  }
}

bool DL::App::hasEnabledPostProcessEffects() const {
  if (!postProcessStack_.enabled) {
    return false;
  }
  for (const auto &effect : postProcessStack_.effects) {
    if (effect.enabled && effect.pipeline.valid()) {
      return true;
    }
  }
  return false;
}

DL::App::PostProcessEffect *
DL::App::findPostProcessEffect(std::string_view name) {
  for (auto &effect : postProcessStack_.effects) {
    if (effect.name == name) {
      return &effect;
    }
  }
  return nullptr;
}

const DL::App::PostProcessEffect *
DL::App::findPostProcessEffect(std::string_view name) const {
  for (const auto &effect : postProcessStack_.effects) {
    if (effect.name == name) {
      return &effect;
    }
  }
  return nullptr;
}

DL::UniformValue *DL::App::findEffectUniform(PostProcessEffect &effect,
                                             std::string_view name) {
  for (auto &uniform : effect.uniforms) {
    if (uniform.name == name) {
      return &uniform;
    }
  }
  return nullptr;
}

const DL::UniformValue *
DL::App::findEffectUniform(const PostProcessEffect &effect,
                           std::string_view name) const {
  for (const auto &uniform : effect.uniforms) {
    if (uniform.name == name) {
      return &uniform;
    }
  }
  return nullptr;
}

void DL::App::renderScenePass(const FrameContext &ctx,
                              std::uint32_t framebufferWidth,
                              std::uint32_t framebufferHeight) {
  const bool usePostProcess =
      hasEnabledPostProcessEffects() && sceneRenderTarget_.valid();
  FramePassDesc passDesc{
      .passId = RenderPassId::Opaque,
      .target = usePostProcess ? sceneRenderTarget_ : RenderTargetHandle{},
      .clearColor = {0.78f, 0.84f, 0.80f, 1.0f},
      .clearFlags = ClearFlags::ColorDepth,
      .depthMode = DepthMode::Less,
  };

  if (!usePostProcess) {
    renderDevice_->setViewport(framebufferWidth, framebufferHeight);
  }
  renderDevice_->beginFrame(passDesc);
  RenderQueue renderQueue;
  FrameContext renderCtx = ctx;
  renderCtx.renderQueue = &renderQueue;
  scene_->render(renderCtx);
  lastRenderQueueStats_ = renderQueue.flush(
      *renderDevice_, {.cullingEnabled = renderCullingEnabled_});
}

void DL::App::renderPostProcessPass(const FrameContext &ctx,
                                    std::uint32_t framebufferWidth,
                                    std::uint32_t framebufferHeight) {
  if (!hasEnabledPostProcessEffects() || !postProcessQuad_.valid() ||
      !sceneRenderTarget_.valid()) {
    return;
  }

  std::vector<const PostProcessEffect *> enabledEffects;
  enabledEffects.reserve(postProcessStack_.effects.size());
  for (const auto &effect : postProcessStack_.effects) {
    if (effect.enabled && effect.pipeline.valid()) {
      enabledEffects.push_back(&effect);
    }
  }
  if (enabledEffects.empty()) {
    return;
  }

  TextureHandle inputTexture =
      renderDevice_->getRenderTargetColorTexture(sceneRenderTarget_);
  std::size_t scratchIndex = 0;
  for (std::size_t effectIndex = 0; effectIndex < enabledEffects.size();
       ++effectIndex) {
    const auto &effect = *enabledEffects[effectIndex];
    const bool isLastEffect = effectIndex + 1 == enabledEffects.size();
    if (isLastEffect) {
      renderDevice_->setViewport(framebufferWidth, framebufferHeight);
    }
    renderDevice_->beginFrame(
        {.passId = RenderPassId::PostProcess,
         .target = isLastEffect ? RenderTargetHandle{}
                                : postProcessTargets_[scratchIndex],
         .clearColor = {0.0f, 0.0f, 0.0f, 1.0f},
         .clearFlags = ClearFlags::Color,
         .depthMode = DepthMode::Disabled});

    DrawCommand command;
    command.mesh = postProcessQuad_;
    command.pipeline = effect.pipeline;
    command.texture = inputTexture;
    command.pass = RenderPassId::PostProcess;
    command.uniforms = effect.uniforms;
    command.uniforms.push_back(UniformValue::makeVec2(
        "screenSize", glm::vec2(framebufferWidth, framebufferHeight)));
    command.uniforms.push_back(
        UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
    renderDevice_->draw(command);

    if (!isLastEffect) {
      inputTexture = renderDevice_->getRenderTargetColorTexture(
          postProcessTargets_[scratchIndex]);
      scratchIndex = 1 - scratchIndex;
    }
  }
}

void DL::App::onClick(int button, int action, int /*mod*/) {
#ifdef USE_IMGUI
  const bool imguiWantsMouse =
      ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
#else
  const bool imguiWantsMouse = false;
#endif

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    if (imguiWantsMouse) {
      return;
    }
    double x, y;
    glfwGetCursorPos(window_, &x, &y);
    if (scene_) {
      const glm::vec2 sceneMousePosition =
          mapMousePositionToScene({static_cast<float>(x), static_cast<float>(y)},
                                  getWindowSize());
      scene_->onClick(sceneMousePosition.x, sceneMousePosition.y);
    }
  }
}

void DL::App::onKey(int key, int scancode, int action, int mod) {
#ifdef USE_IMGUI
  const bool imguiWantsKeyboard = ImGui::GetCurrentContext() != nullptr &&
                                  ImGui::GetIO().WantCaptureKeyboard;
#else
  const bool imguiWantsKeyboard = false;
#endif

  if (action != GLFW_RELEASE) {
    if (action == GLFW_PRESS) {
      if (!imguiWantsKeyboard && key == GLFW_KEY_RIGHT) {
        sceneManager_.next();
        Logger::instance().logEvent(LogLevel::Info, "scene", "scene_next");
        loadCurrentScene();
        return;
      }
      if (!imguiWantsKeyboard && key == GLFW_KEY_LEFT) {
        sceneManager_.previous();
        Logger::instance().logEvent(LogLevel::Info, "scene", "scene_previous");
        loadCurrentScene();
        return;
      }
    }
    return;
  }

  if (!imguiWantsKeyboard && scene_) {
    scene_->onKey(key);
  }
}

void DL::App::basisInit() {
  basist::basisu_transcoder_init();
  codebook_ = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
  LogInfo("BasisU transcoder initialized");
  Logger::instance().logEvent(LogLevel::Info, "assets", "basisu_initialized");
}

void DL::App::onScreenSizeChanged(int width, int height) {
  lastObservedWindowSize_ = {width, height};
  if (scene_) {
    scene_->onScreenSizeChanged({width, height});
  }
}

void DL::App::onFramebufferSizeChanged(int width, int height) {
  lastObservedFramebufferSize_ = {width, height};
  if (renderDevice_) {
    renderDevice_->setViewport(static_cast<std::uint32_t>(width),
                               static_cast<std::uint32_t>(height));
  }
  if (scene_) {
    scene_->onFramebufferSizeChanged({width, height});
  }
}

void DL::App::syncObservedWindowSizes() {
  if (window_ == nullptr) {
    return;
  }

  int windowWidth = 0;
  int windowHeight = 0;
  glfwGetWindowSize(window_, &windowWidth, &windowHeight);
  const glm::ivec2 windowSize{windowWidth, windowHeight};
  if (windowSize != lastObservedWindowSize_) {
    onScreenSizeChanged(windowWidth, windowHeight);
  }

  int framebufferWidth = 0;
  int framebufferHeight = 0;
  glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
  const glm::ivec2 framebufferSize{framebufferWidth, framebufferHeight};
  if (framebufferSize != lastObservedFramebufferSize_) {
    onFramebufferSizeChanged(framebufferWidth, framebufferHeight);
  }
}

glm::vec2 DL::App::getWindowSize() const {
  int width = static_cast<int>(screen_width);
  int height = static_cast<int>(screen_height);
  if (window_) {
    glfwGetWindowSize(window_, &width, &height);
  }
  return {width, height};
}

glm::vec2 DL::App::getFramebufferSize() const {
  int width = static_cast<int>(screen_width);
  int height = static_cast<int>(screen_height);
  if (window_) {
    glfwGetFramebufferSize(window_, &width, &height);
  }
  return {width, height};
}

void DL::App::calculateDeltaTime() {
  float currentFrameTime = static_cast<float>(glfwGetTime());
  startFrameTime_ = currentFrameTime;
  deltaTime_ = currentFrameTime - lastFrameTime_;
  lastFrameTime_ = currentFrameTime;
}

void DL::App::setFrameRateLimit(float fps) {
  desiredFrameTime_ = fps > 0.0f ? 1.0f / fps : 0.0f;
}
