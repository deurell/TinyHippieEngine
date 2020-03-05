#include "app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "transcoder/basisu_transcoder.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

void renderLoopCallback(void *arg) { static_cast<App *>(arg)->render(); }

void App::init() {
  glfwInit();
  basisInit();
}

int App::run() {
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

  mWindow = glfwCreateWindow(screen_width, screen_height, "tiny hippie engine",
                             nullptr, nullptr);
  if (mWindow == nullptr) {
    std::cout << "window create failed";
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(mWindow);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "glad init failed";
    return -1;
  }

  glViewport(0, 0, screen_width, screen_height);

  // mTexture =
  //    std::make_unique<Texture>("Resources/sup.basis", *mCodebook, GL_RGB);

#ifdef Emscripten
  std::string glslVersionString = "#version 300 es\n";
#else
  std::string glslVersionString = "#version 330 core\n";
#endif

  mLampShader = std::make_unique<Shader>(
      "Shaders/lamp.vert", "Shaders/lamp.frag", glslVersionString);

  mLightingShader = std::make_unique<Shader>(
      "Shaders/model.vert", "Shaders/model.frag", glslVersionString);

  mModel = std::make_unique<Model>("Resources/bridge.obj", mCodebook.get());

  float cubeVertices[] = {
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
      0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
      0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
      1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
      -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
      0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
      -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
      0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
      0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
      -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
      -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
      1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
      -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
      1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

  glGenVertexArrays(1, &mCubeVAO);
  glBindVertexArray(mCubeVAO);
  unsigned int VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glBindVertexArray(0);

  // lamp
  glGenVertexArrays(1, &mLightVAO);
  glBindVertexArray(mLightVAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
  ImGui_ImplOpenGL3_Init(glslVersionString.c_str());

  mCamera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 7.0f));
  mCamera->lookAt({0.0f, 0.0f, 0.0f});

#ifdef Emscripten
  emscripten_set_main_loop_arg(&renderLoopCallback, this, -1, 1);
#else
  while (!glfwWindowShouldClose(mWindow)) {
    render();
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(mWindow);
#endif

  glfwTerminate();
  return 0;
}

void App::render() {
  float currentFrame = glfwGetTime();
  mDeltaTime = currentFrame - mLastFrame;
  mLastFrame = currentFrame;

  processInput(mWindow);

  glClearColor(0.52f, 0.81f, .92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("tiny hippie engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("iTime: %.1f", glfwGetTime());
  ImGui::InputFloat3("mod", &mModelTranslate.x);
  ImGui::InputFloat3("cam", &mCamera->mPosition.x);
  ImGui::InputFloat3("light", &mPointLightPositions[0].x);
  ImGui::InputFloat3("light2", &mPointLightPositions[1].x);
  float dot =
      glm::dot(mCamera->mOrientation * glm::vec3(0, 1, 0), glm::vec3(0, -1, 0));
  ImGui::InputFloat("dot", &dot);
  ImGui::End();

  mLightingShader->use();

  mLightingShader->setFloat("iTime", glfwGetTime());
  mLightingShader->setVec3f("dirLight.direction", -0.2f, -1.0f, -0.3f);
  mLightingShader->setVec3f("dirLight.ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
  mLightingShader->setVec3f("dirLight.specular", 0.6f, 0.6f, 0.6f);

  mLightingShader->setVec3f("pointLights[0].position", mPointLightPositions[0]);
  mLightingShader->setVec3f("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
  mLightingShader->setVec3f("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
  mLightingShader->setFloat("pointLights[0].constant", 1.0f);
  mLightingShader->setFloat("pointLights[0].linear", 0.09);
  mLightingShader->setFloat("pointLights[0].quadratic", 0.032);

  mLightingShader->setVec3f("pointLights[1].position", mPointLightPositions[1]);
  mLightingShader->setVec3f("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("pointLights[1].diffuse", 0.4f, 0.4f, 0.4f);
  mLightingShader->setVec3f("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
  mLightingShader->setFloat("pointLights[1].constant", 1.0f);
  mLightingShader->setFloat("pointLights[1].linear", 0.09);
  mLightingShader->setFloat("pointLights[1].quadratic", 0.032);

  mPointLightPositions[0].x = 2 * glm::cos(-1.5 * glfwGetTime());
  mPointLightPositions[0].z = 4.0 * glm::cos(1.3 * glfwGetTime());
  mPointLightPositions[0].y = 1 + .5 * glm::sin(-1.1 * glfwGetTime());

  mPointLightPositions[1].x = 1.5 * glm::sin(0.9 * glfwGetTime());
  mPointLightPositions[1].z = 1.5 * glm::cos(1.3 * glfwGetTime());
  mPointLightPositions[1].y = 1.0 + 0.8 * glm::sin(-0.2 * glfwGetTime());

  // view/projection transformations
  glm::mat4 projection =
      mCamera->getPerspectiveTransform(45.0, screen_width / screen_height);

  glm::mat4 view = mCamera->getViewMatrix();
  mLightingShader->setMat4f("projection", projection);
  mLightingShader->setMat4f("view", view);

  // render the loaded model
  glm::mat4 m2 = glm::mat4(1.0f);
  m2 = glm::scale(m2, glm::vec3(0.5f, 0.5f, 0.5f));
  m2 = glm::translate(m2, mModelTranslate);
  m2 = glm::rotate(m2, glm::radians<float>(glfwGetTime()) * 10,
                   glm::vec3(0.0, 1.0, 0.0));

  mLightingShader->setMat4f("model", m2);
  mModel->Draw(*mLightingShader);

  glBindVertexArray(mCubeVAO);
  mLampShader->use();
  mLampShader->setMat4f("projection", projection);
  mLampShader->setMat4f("view", view);

  glm::mat4 mod = glm::mat4(1.0);
  glBindVertexArray(mLightVAO);
  for (unsigned int i = 0; i < point_light_count; i++) {
    mod = glm::mat4(1.0f);
    mod = glm::translate(mod, mPointLightPositions[i]);
    mod = glm::scale(mod, glm::vec3(0.1f));
    mLampShader->setMat4f("model", mod);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(mWindow, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);
  glfwSwapBuffers(mWindow);
  glfwPollEvents();
}

void App::processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  const float cameraSpeed = 2.0f * mDeltaTime;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    mCamera->translate(glm::vec3(0, 0, -cameraSpeed));
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    mCamera->translate(glm::vec3(0, 0, cameraSpeed));
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    mCamera->translate(glm::vec3(-cameraSpeed, 0, 0));
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    mCamera->translate(glm::vec3(cameraSpeed, 0, 0));
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    mCamera->translate(glm::vec3(0, cameraSpeed, 0));
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    mCamera->translate(glm::vec3(0, -cameraSpeed, 0));
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
    mCamera->lookAt({0.0f, 0.0f, 0.0f});
  }
}

void App::basisInit() {
  basist::basisu_transcoder_init();
  mCodebook = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
}
