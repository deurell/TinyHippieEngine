#include "truetypescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h "

TrueTypeScene::TrueTypeScene(std::string glslVersion)
    : mGlslVersionString(glslVersion) {}

void TrueTypeScene::initFont() {
  std::ifstream iStream("Resources/C64_Pro-STYLE.ttf", std::ios::binary);
  iStream.seekg(0, iStream.end);
  const int size = iStream.tellg();
  iStream.seekg(0, iStream.beg);
  char *fontData = new char[size];

  iStream.read((char *)fontData, size);
  iStream.close();

  auto atlasData =
      std::make_unique<uint8_t[]>(font.atlasWidth * font.atlasHeight);

  font.charInfo = std::make_unique<stbtt_packedchar[]>(font.charCount);

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(), font.atlasWidth,
                       font.atlasHeight, 0, 1, nullptr)) {
    std::cout << "init font failed.";
  }
  stbtt_PackSetOversampling(&context, font.oversampleX, font.oversampleY);

  if (!stbtt_PackFontRange(
          &context, reinterpret_cast<const unsigned char *>(fontData), 0,
          font.size, font.firstChar, font.charCount, font.charInfo.get())) {
    std::cout << "pack font failed";
  }

  stbtt_PackEnd(&context);
  glGenTextures(1, &font.texture);
  glBindTexture(GL_TEXTURE_2D, font.texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font.atlasWidth, font.atlasHeight, 0,
               GL_RED, GL_UNSIGNED_BYTE, atlasData.get());
  glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
  glGenerateMipmap(GL_TEXTURE_2D);
  delete[] fontData;
}

void TrueTypeScene::init() {
  initFont();
  mShader = std::make_unique<DL::Shader>("Shaders/tt.vert", "Shaders/tt.frag",
                                         mGlslVersionString);

  float vertices[] = {
      // positions        // colors         // texture coords
      1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
  };

  unsigned int indices[] = {0, 1, 3, 1, 2, 3};

  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVBO);
  glGenBuffers(1, &mEBO);

  glBindVertexArray(mVAO);

  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
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
  glBindVertexArray(0);

  mShader->setInt("texture1", 0);
  mShader->use();
}

void TrueTypeScene::render(float delta) {
  mDelta = delta;

  glClearColor(1.0, 0., 0., 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, font.texture);

  mShader->use();
  mShader->setFloat("iTime", (float)glfwGetTime());
  mShader->setInt("texture1", 0);

  glBindVertexArray(mVAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("true type demo");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::End();
}

void TrueTypeScene::onKey(int key){};

void TrueTypeScene::onScreenSizeChanged(glm::vec2 size) { mScreenSize = size; };
