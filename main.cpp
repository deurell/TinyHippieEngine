#define STB_IMAGE_IMPLEMENTATION

#include "camera.h"
#include "shader.h"
#include "texture.h"
#include <GLFW/glfw3.h>

#ifdef Emscripten
#include <emscripten.h>
#endif

#include <glad/glad.h>
#include <iostream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "transcoder/basisu_transcoder.h"
#include <glm/glm.hpp>
#include <memory>

void onResize(GLFWwindow *window, int height, int width);
void processInput(GLFWwindow *window);
void renderLoop();
void basisInit();

GLFWwindow *m_window;
std::unique_ptr<Texture> mTexture;
std::unique_ptr<Shader> mShader;
std::unique_ptr<Camera> mCamera;
std::unique_ptr<basist::etc1_global_selector_codebook> m_codebook;

unsigned int m_VAO;
constexpr float screenWidth = 1024;
constexpr float screenHeight = 768;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 model_translate;

int main() {
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
  glfwSetFramebufferSizeCallback(m_window, onResize);

  float vertices[] = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

  const int vertexCount = sizeof(vertices) / sizeof(float) * 5;

  unsigned int VBO;
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  mTexture =
      std::make_unique<Texture>("Resources/sup.basis", *m_codebook, GL_RGB);

#ifdef Emscripten
  std::string glslVersionString = "#version 300 es\n";
#else
  std::string glslVersionString = "#version 330 core\n";
#endif
  mShader = std::make_unique<Shader>("Shaders/shader.vert",
                                     "Shaders/shader.frag", glslVersionString);
  mShader->use();
  mShader->setInt("texture1", 0);

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

void renderLoop() {
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  processInput(m_window);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("tiny engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("iTime: %.1f", ImGui::GetTime());
  ImGui::InputFloat3("mod", &model_translate.x);
  ImGui::InputFloat3("cam", &mCamera->mPosition.x);
  ImGui::Text("tex");
  ImGui::Image((ImTextureID)mTexture->mId, ImVec2(100, 100));

  ImGui::End();

  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexture->mId);

  mShader->use();
  mShader->setFloat("iTime", (float)glfwGetTime());

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, model_translate);
  // model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 0.0f,
  // 0.0f)); model = glm::scale(model, glm::vec3(100.0,100.0, 100.0));
  mShader->setMat4f("model", model);

  glm::mat4 view = mCamera->getViewMatrix();
  mShader->setMat4f("view", view);

  glm::mat4 projection = glm::mat4(1.0f);
  projection =
      mCamera->getPerspectiveTransform(45.0, screenWidth / screenHeight);
  // projection = mCamera->getOrtoTransform(0.0f, screenWidth, 0.0f,
  // screenHeight);
  mShader->setMat4f("projection", projection);

  glBindVertexArray(m_VAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(m_window, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);
  glfwSwapBuffers(m_window);
  glfwPollEvents();
}

void onResize(GLFWwindow * /*window*/, int /*height*/, int /*width*/) {}

void processInput(GLFWwindow *window) {
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

void basisInit() {
  basist::basisu_transcoder_init();
  m_codebook = std::make_unique<basist::etc1_global_selector_codebook>(
      basist::g_global_selector_cb_size, basist::g_global_selector_cb);
}
