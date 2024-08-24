#include "c64scene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <iostream>

C64Scene::C64Scene(std::string_view glslVersion,
                   basist::etc1_global_selector_codebook *codeBook)
    : glslVersionString_(glslVersion), codeBook_(codeBook) {
  audioPlayer_ = std::make_unique<AudioPlayer>();
  audioPlayer_->load(audio_unlock, "Resources/unlock.wav");
  audioPlayer_->play(audio_unlock);
}

void C64Scene::init() {
  shader_ = std::make_unique<DL::Shader>("Shaders/c64.vert", "Shaders/c64.frag",
                                         glslVersionString_);

  float vertices[] = {
      // positions        // colors         // texture coords
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
  };

  unsigned int indices[] = {0, 1, 3, 1, 2, 3};

  glGenVertexArrays(1, &VAO_);
  glGenBuffers(1, &VBO_);
  glGenBuffers(1, &EBO_);

  glBindVertexArray(VAO_);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
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

  texture_ = std::make_unique<DL::Texture>("Resources/sup.basis", GL_TEXTURE0, *codeBook_);
  glBindVertexArray(0);

  shader_->use();
  shader_->setInt("texture1", 0);
}

void C64Scene::update(float delta) {}

void C64Scene::render(float delta) {
  delta_ = delta;
  glClearColor(0.52f, 0.81f, .92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glClearColor(0.3, 0.3, 0.3, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_->mId);

  shader_->use();
  shader_->setFloat("iTime", (float)glfwGetTime());

  glm::mat4 transform = glm::mat4(1.0f);
  transform = glm::rotate(transform, glm::radians<float>(0),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  shader_->setMat4f("transform", transform);

  glBindVertexArray(VAO_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

#ifdef USE_IMGUI
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("tiny hippie engine");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
#endif
}

void C64Scene::onClick(double x, double y) {}

void C64Scene::onKey(int key) {}

void C64Scene::onScreenSizeChanged(glm::vec2 size) { screenSize_ = size; }
