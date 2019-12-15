#define STB_IMAGE_IMPLEMENTATION

#include "shader.h"
#include "texture.h"
#include <GLFW/glfw3.h>

#if Emscripten
#include <emscripten.h>
#endif

#include <fstream>
#include <glad/glad.h>
#include <iostream>

#include "logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void onResize(GLFWwindow *window, int height, int width);
void processInput(GLFWwindow *window);
void mainLoop();

GLFWwindow *m_window;
Texture *m_texture1;
Shader *m_shader;
unsigned int m_VAO;

int main() {
  glfwInit();

#if __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  m_window = glfwCreateWindow(1024, 768, "gfxlab", nullptr, nullptr);
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

  glViewport(0, 0, 1024, 768);
  glfwSetFramebufferSizeCallback(m_window, onResize);

  float vertices[] = {
      // positions        // colors         // texture coords
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
  };

  unsigned int indices[] = {0, 1, 3, 1, 2, 3};

  unsigned int VBO, EBO;
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
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

  m_texture1 = new Texture("Resources/sup.jpg", GL_RGB);

#if Emscripten
  std::string glslVersionString = "#version 300 es\n";
#else
  std::string glslVersionString = "#version 330 core\n";
#endif
  m_shader = new Shader("Shaders/shader.vert", "Shaders/shader.frag",
                        glslVersionString);

  m_shader->use();
  m_shader->setInt("texture1", 0);

#if Emscripten
  emscripten_set_main_loop(mainLoop, 0, true);
#else
  while (!glfwWindowShouldClose(m_window)) {
    mainLoop();
  }
  glfwDestroyWindow(m_window);
#endif

  delete m_shader;
  delete m_texture1;
  glfwTerminate();
  return 0;
}

void mainLoop() {
  processInput(m_window);

  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_texture1->mId);

  m_shader->use();
  m_shader->setFloat("iTime", (float)glfwGetTime());

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::rotate(transform, glm::radians<float>(0),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  m_shader->setMat4f("transform", transform);
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  int frameWidth, frameHeight;
  glfwGetFramebufferSize(m_window, &frameWidth, &frameHeight);
  glViewport(0, 0, frameWidth, frameHeight);
  glfwSwapBuffers(m_window);
  glfwPollEvents();
}

void onResize(GLFWwindow * /*window*/, int /*height*/, int /*width*/) {}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
    glfwSetWindowShouldClose(window, true);
  }
}
