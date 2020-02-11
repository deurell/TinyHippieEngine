#include "app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "transcoder/basisu_transcoder.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION

int App::run() {
  glfwInit();
  basisInit();

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

  m_window =
      glfwCreateWindow(screenWidth, screenHeight, "gfxlab", nullptr, nullptr);
  if (m_window == nullptr) {
    std::cout << "window create failed";
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "glad init failed";
    return -1;
  }

  glViewport(0, 0, screenWidth, screenHeight);

  mTexture =
      std::make_unique<Texture>("Resources/sup.basis", *m_codebook, GL_RGB);

#ifdef Emscripten
  std::string glslVersionString = "#version 300 es\n";
#else
  std::string glslVersionString = "#version 330 core\n";
#endif

  mLampShader = std::make_unique<Shader>(
      "Shaders/lamp.vert", "Shaders/lamp.frag", glslVersionString);

  mLightingShader = std::make_unique<Shader>(
      "Shaders/light.vert", "Shaders/light.frag", glslVersionString);

  float vertices[] = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
      0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
      0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
      0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
      -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
      1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
      0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
      0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
      0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
      0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};

  // cube
  unsigned int VBO;
  glGenVertexArrays(1, &mCubeVAO);
  glBindVertexArray(mCubeVAO);
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);

  // lamp
  glGenVertexArrays(1, &mLightVAO);
  glBindVertexArray(mLightVAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init(glslVersionString.c_str());

  mCamera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 5.0f));
  mCamera->lookAt({0.0f, 0.0f, 0.0f});

#ifdef Emscripten
  emscripten_set_main_loop(renderLoop, 0, true);
#else
  while (!glfwWindowShouldClose(m_window)) {
    renderLoop();
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(m_window);
#endif

  glfwTerminate();
  return 0;
}

void App::renderLoop() {
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  processInput(m_window);

  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("tiny engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("iTime: %.1f", ImGui::GetTime());
  ImGui::InputFloat3("mod", &model_translate.x);
  ImGui::InputFloat3("cam", &mCamera->mPosition.x);
  ImGui::InputFloat3("light", &mLightPos.x);
  ImGui::ColorPicker3("obj col", &obj_color.x);
  ImGui::Text("tex");
  ImGui::Image((ImTextureID)mTexture->mId, ImVec2(100, 100));
  ImGui::End();

  mLightingShader->use();
  mLightingShader->setVec3f("objectColor", obj_color);
  mLightingShader->setVec3f("lightColor", 1.0f, 1.0f, 1.0f);
  mLightingShader->setVec3f("lightPos", mLightPos);
  mLightingShader->setVec3f("viewPos", mCamera->mPosition);
  glm::mat4 projection =
      mCamera->getPerspectiveTransform(45.0, screenWidth / screenHeight);
  glm::mat4 view = mCamera->getViewMatrix();
  mLightingShader->setMat4f("projection", projection);
  mLightingShader->setMat4f("view", view);
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, model_translate);
  model = glm::rotate<float>(model, ImGui::GetTime(), {0.0, 1.0, 0.0});
  model = glm::rotate<float>(model, ImGui::GetTime() * -1.4, {1.0, 1.0, 0.0});
  mLightingShader->setMat4f("model", model);
  glBindVertexArray(mCubeVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  mLampShader->use();
  mLampShader->setMat4f("projection", projection);
  mLampShader->setMat4f("view", view);
  model = glm::mat4(1.0);
  model = glm::translate(model, mLightPos);
  model = glm::scale(model, glm::vec3(0.2f));
  mLampShader->setMat4f("model", model);
  glBindVertexArray(mLightVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(m_window, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);
  glfwSwapBuffers(m_window);
  glfwPollEvents();
}

void App::processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  const float cameraSpeed = 5.0f * deltaTime; // adjust accordingly
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    mCamera->mPosition.z -= cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    mCamera->mPosition.z += cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    mCamera->mPosition.x -= cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    mCamera->mPosition.x += cameraSpeed;
}

void App::basisInit() {
  basist::basisu_transcoder_init();
  m_codebook = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
}
