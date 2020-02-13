#include "app.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "transcoder/basisu_transcoder.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION

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

  mWindow =
      glfwCreateWindow(screen_width, screen_height, "gfxlab", nullptr, nullptr);
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

  mTexture =
      std::make_unique<Texture>("Resources/sup.basis", *mCodebook, GL_RGB);

#ifdef Emscripten
  std::string glslVersionString = "#version 300 es\n";
#else
  std::string glslVersionString = "#version 330 core\n";
#endif

  mLampShader = std::make_unique<Shader>(
      "Shaders/lamp.vert", "Shaders/lamp.frag", glslVersionString);

  mLightingShader = std::make_unique<Shader>(
      "Shaders/light.vert", "Shaders/light.frag", glslVersionString);

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

  mCamera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 5.0f));
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

  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("tiny engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("iTime: %.1f", ImGui::GetTime());
  ImGui::InputFloat3("mod", &mModelTranslate.x);
  ImGui::InputFloat3("cam", &mCamera->mPosition.x);
  ImGui::InputFloat3("light", &mLightPos.x);
  ImGui::Text("tex");
  ImGui::Image((ImTextureID)mTexture->mId, ImVec2(100, 100));
  ImGui::End();

  glm::vec3 pointLightPositions[] = {
      glm::vec3(0.7f, 0.2f, 2.0f), glm::vec3(2.3f, -3.3f, -4.0f),
      glm::vec3(-4.0f, 2.0f, -12.0f), glm::vec3(0.0f, 0.0f, -3.0f)};

  mLightingShader->use();
  mLightingShader->setVec3f("material.specular", 0.5f, 0.5f, 0.5f);
  mLightingShader->setInt("material.diffuse", 0);
  mLightingShader->setFloat("material.shininess", 32.0f);

  mLightingShader->setVec3f("dirLight.direction", -0.2f, -1.0f, -0.3f);
  mLightingShader->setVec3f("dirLight.ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
  mLightingShader->setVec3f("dirLight.specular", 0.5f, 0.5f, 0.5f);

  mLightingShader->setVec3f("pointLights[0].position", pointLightPositions[0]);
  mLightingShader->setVec3f("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
  mLightingShader->setVec3f("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
  mLightingShader->setFloat("pointLights[0].constant", 1.0f);
  mLightingShader->setFloat("pointLights[0].linear", 0.09);
  mLightingShader->setFloat("pointLights[0].quadratic", 0.032);

  mLightingShader->setVec3f("pointLights[1].position", pointLightPositions[1]);
  mLightingShader->setVec3f("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
  mLightingShader->setVec3f("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
  mLightingShader->setVec3f("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
  mLightingShader->setFloat("pointLights[1].constant", 1.0f);
  mLightingShader->setFloat("pointLights[1].linear", 0.09);
  mLightingShader->setFloat("pointLights[1].quadratic", 0.032);

  mLightingShader->setVec3f("viewPos", mCamera->mPosition);
  glm::mat4 projection =
      mCamera->getPerspectiveTransform(45.0, screen_width / screen_height);
  glm::mat4 view = mCamera->getViewMatrix();
  mLightingShader->setMat4f("projection", projection);
  mLightingShader->setMat4f("view", view);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexture->mId);

  glBindVertexArray(mCubeVAO);

  glm::vec3 cubePositions[] = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f)};

  for (unsigned int i = 0; i < 8; i++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cubePositions[i]);
    float angle = 32.0f * i;
    model =
        glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

    mLightingShader->setMat4f("model", model);

    glDrawArrays(GL_TRIANGLES, 0, 36);
  }

  mLampShader->use();
  mLampShader->setMat4f("projection", projection);
  mLampShader->setMat4f("view", view);
  glm::mat4 model = glm::mat4(1.0);
  model = glm::translate(model, mLightPos);
  model = glm::scale(model, glm::vec3(0.2f));
  mLampShader->setMat4f("model", model);
  glBindVertexArray(mLightVAO);
  for (unsigned int i = 0; i < 2; i++) {
    model = glm::mat4(1.0f);
    model = glm::translate(model, pointLightPositions[i]);
    model = glm::scale(model, glm::vec3(0.2f));
    mLampShader->setMat4f("model", model);
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

  const float cameraSpeed = 5.0f * mDeltaTime;
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
  mCodebook = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
}
