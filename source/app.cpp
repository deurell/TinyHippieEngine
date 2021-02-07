#include "app.h"
#include "c64scene.h"
#include "demoscene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "truetypescene.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

#include "AL/al.h"
#include "AL/alc.h"

void renderLoopCallback(void *arg) { static_cast<DL::App *>(arg)->render(); }

void DL::App::init() {
  glfwInit();
  basisInit();
#ifdef __EMSCRIPTEN__
  mGlslVersionString = "#version 300 es\n";
#else
  mGlslVersionString = "#version 330 core\n";
#endif
  mScene = std::make_unique<TrueTypeScene>(mGlslVersionString);
}

int DL::App::run() {
  init();

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

  mWindow = glfwCreateWindow(screen_width, screen_height, windows_title,
                             nullptr, nullptr);
  if (mWindow == nullptr) {
    std::cout << "window create failed";
    glfwTerminate();
    return -1;
  }
  glfwSetInputMode(mWindow, GLFW_STICKY_KEYS, GLFW_TRUE);

  glfwMakeContextCurrent(mWindow);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "glad init failed";
    return -1;
  }

  glViewport(0, 0, screen_width, screen_height);

#ifdef USE_IMGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsLight();
  ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
  ImGui_ImplOpenGL3_Init(mGlslVersionString.c_str());
#endif

  mScene->init();
  mScene->onScreenSizeChanged({screen_width, screen_height});

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(&renderLoopCallback, this, -1, 1);
#else
  while (!glfwWindowShouldClose(mWindow)) {
    render();
  }
#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
#endif

  glfwDestroyWindow(mWindow);
#endif

  glfwTerminate();
  return 0;
}

void DL::App::render() {
  auto currentFrame = static_cast<float>(glfwGetTime());
  mDeltaTime = currentFrame - mLastFrame;
  mLastFrame = currentFrame;

  mScene->render(mDeltaTime);

#ifdef USE_IMGUI
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

  processInput(mWindow);

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(mWindow, &frameWidth, &frameHeight);
  mScene->onScreenSizeChanged({frameWidth, frameHeight});
  glViewport(0, 0, frameWidth, frameHeight);
  glfwSwapBuffers(mWindow);
  glfwPollEvents();
}

void DL::App::processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    mScene = std::make_unique<DemoScene>(mGlslVersionString, mCodebook.get());
    mScene->init();
  }

  for (int i = GLFW_KEY_A; i <= GLFW_KEY_Z; i++) {
    if (glfwGetKey(window, i) == GLFW_PRESS) {
      mScene->onKey(i);
    }
  }
}

void DL::App::basisInit() {
  basist::basisu_transcoder_init();
  mCodebook = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
}
