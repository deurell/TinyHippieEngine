#include "app.h"
#include "GLFW/glfw3.h"
#include "c64scene.h"
#include "demoscene.h"
#include "glosifyscene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "introscene.h"
#include "nodeexamplescene.h"
#include "particlescene.h"
#include "quicknodescene.h"
#include "scenemanager.h"
#include "simplescene.h"
#include "truetypescene.h"
#include "wildcopperscene.h"
#include "logger.h"
#include <iostream>
#include <thread>

void renderloop_callback(void *arg) {
  auto app = static_cast<DL::App *>(arg);
  app->update();
  app->render();
}

void mouseclick_callback(GLFWwindow *window, int button, int action, int mod) {
  if (auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window))) {
    app->onClick(button, action, mod);
  }
}

void keyclick_callback(GLFWwindow *window, int key, int scancode, int action,
                       int mods) {
  if (auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window))) {
    app->onKey(key, scancode, action, mods);
  }
}

void window_size_callback(GLFWwindow *window, int width, int height) {
  auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window));
  app->onScreenSizeChanged(width, height);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  auto *app = static_cast<DL::App *>(glfwGetWindowUserPointer(window));
  app->onFramebufferSizeChanged(width, height);
}

bool DL::App::init() {
  if (!glfwInit()) {
    LogError("GLFW initialization failed");
    return false;
  }
  basisInit();
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

int DL::App::run() {
  if (!init()) {
    return EXIT_FAILURE;
  }

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
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetMouseButtonCallback(window_, mouseclick_callback);
  glfwSetKeyCallback(window_, keyclick_callback);
  glfwSetWindowSizeCallback(window_, window_size_callback);
  glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);
  startFrameTime_ = lastFrameTime_ = static_cast<float>(glfwGetTime());

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    LogError("GLAD initialization failed");
    return EXIT_FAILURE;
  }

  renderDevice_ = createOpenGLRenderDevice(glslVersionString_);

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(window_, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);

#ifdef USE_IMGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsLight();
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
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
  scene_.reset();
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
#endif
  glfwDestroyWindow(window_);
#endif
  glfwTerminate();
  return EXIT_SUCCESS;
}

void DL::App::update() {
  calculateDeltaTime();
  if (window_) {
    processInput(window_);
  }
  if (!scene_) return;
  scene_->update({deltaTime_, glfwGetTime()});
}

void DL::App::render() {
  if (!scene_) return;
  scene_->render({deltaTime_, glfwGetTime()});

#ifdef USE_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(window_, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);
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
}

void DL::App::loadSimpleScene() {
  scene_ = replacePreparedScene(
      std::move(scene_),
      std::make_unique<SimpleScene>(glslVersionString_, renderDevice_.get()),
      getWindowSize(), getFramebufferSize());
}

void DL::App::loadCurrentScene() {
  auto previousScene = std::move(scene_);
  scene_ = replacePreparedScene(std::move(previousScene),
                                sceneManager_.createCurrent(glslVersionString_),
                                getWindowSize(), getFramebufferSize());
  if (!scene_) {
    LogError("Failed to create scene");
  }
}

void DL::App::registerScenes() {
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<QuickNodeScene>(renderDevice_.get());
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<NodeExampleScene>(renderDevice_.get());
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<DemoScene>(glsl, codebook_.get());
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<TrueTypeScene>(glsl);
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<GlosifyScene>(codebook_.get(), renderDevice_.get());
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<IntroScene>(glsl);
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<C64Scene>(glsl, codebook_.get(), renderDevice_.get());
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<ParticleScene>(glsl);
  });
  sceneManager_.registerScene([this](std::string_view glsl) {
    return std::make_unique<WildCopperScene>(glsl);
  });
}

void DL::App::onClick(int button, int action, int /*mod*/) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double x, y;
    glfwGetCursorPos(window_, &x, &y);
    if (scene_) {
      scene_->onClick(static_cast<float>(x), static_cast<float>(y));
    }
  }
}

void DL::App::onKey(int key, int scancode, int action, int mod) {
  if (action != GLFW_RELEASE) {
    if (action == GLFW_PRESS) {
      if (key == GLFW_KEY_RIGHT) {
        sceneManager_.next();
        loadCurrentScene();
        return;
      }
      if (key == GLFW_KEY_LEFT) {
        sceneManager_.previous();
        loadCurrentScene();
        return;
      }
    }
    return;
  }

  if (key == GLFW_KEY_SPACE) {
    loadSimpleScene();
    return;
  }

  if (scene_) {
    scene_->onKey(key);
  }
}

void DL::App::basisInit() {
  basist::basisu_transcoder_init();
  codebook_ = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
  LogInfo("BasisU transcoder initialized");
}

void DL::App::onScreenSizeChanged(int width, int height) {
  if (scene_) {
    scene_->onScreenSizeChanged({width, height});
  }
}

void DL::App::onFramebufferSizeChanged(int width, int height) {
  glViewport(0, 0, width, height);
  if (scene_) {
    scene_->onFramebufferSizeChanged({width, height});
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
