#include "demoscene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <utility>

DemoScene::DemoScene(std::string_view glslVersion,
                     basist::etc1_global_selector_codebook *codeBook)
    : mGlslVersionString(glslVersion), mCodeBook(codeBook) {}

void DemoScene::init() {
  mLampShader = std::make_unique<DL::Shader>(
      "Shaders/lamp.vert", "Shaders/lamp.frag", mGlslVersionString);

  mLightingShader = std::make_unique<DL::Shader>(
      "Shaders/model.vert", "Shaders/model.frag", mGlslVersionString);

  mModel = std::make_unique<DL::Model>("Resources/bridge.obj", mCodeBook);

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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)nullptr);
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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)nullptr);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

  mCamera = std::make_unique<DL::Camera>(glm::vec3(0.0f, 5.0f, 7.0f));
  mCamera->lookAt({0.0f, 0.0f, 0.0f});
}

void DemoScene::render(float delta) {
  auto time = static_cast<float>(glfwGetTime());
  mDelta = delta;
  glClearColor(0.52f, 0.81f, .92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef USE_IMGUI
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
  ImGui::End();
#endif

  mLightingShader->use();

  mLightingShader->setFloat("iTime", time);
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
      mCamera->getPerspectiveTransform(45.0, mScreenSize.x / mScreenSize.y);

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
  for (auto &mPointLightPosition : mPointLightPositions) {
    mod = glm::mat4(1.0f);
    mod = glm::translate(mod, mPointLightPosition);
    mod = glm::scale(mod, glm::vec3(0.1f));
    mLampShader->setMat4f("model", mod);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
}
void DemoScene::onKey(int key) {
  const float cameraSpeed = 1.5f * mDelta;
  if (key == GLFW_KEY_W) {
    mCamera->translate(glm::vec3(0, 0, -cameraSpeed));
  }
  if (key == GLFW_KEY_S) {
    mCamera->translate(glm::vec3(0, 0, cameraSpeed));
  }
  if (key == GLFW_KEY_A) {
    mCamera->translate(glm::vec3(-cameraSpeed, 0, 0));
  }
  if (key == GLFW_KEY_D) {
    mCamera->translate(glm::vec3(cameraSpeed, 0, 0));
  }
  if (key == GLFW_KEY_Q) {
    mCamera->translate(glm::vec3(0, cameraSpeed, 0));
  }
  if (key == GLFW_KEY_E) {
    mCamera->translate(glm::vec3(0, -cameraSpeed, 0));
  }
  if (key == GLFW_KEY_L) {
    mCamera->lookAt({0.0f, 0.0f, 0.0f});
  }
}

void DemoScene::onScreenSizeChanged(glm::vec2 size) { mScreenSize = size; }