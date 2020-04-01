#include "truetypescene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "texture.h"
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <glm/glm.hpp>

TrueTypeScene::TrueTypeScene(std::string glslVersion)
    : mGlslVersionString(glslVersion) {}

void TrueTypeScene::loadFontTexture() {
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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, font.atlasWidth, font.atlasHeight, 0,
               GL_RED, GL_UNSIGNED_BYTE, atlasData.get());
  glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
  glGenerateMipmap(GL_TEXTURE_2D);
  delete[] fontData;
}

GlyphInfo TrueTypeScene::makeGlyphInfo(uint32_t character, float offsetX,
                                       float offsetY) {
  stbtt_aligned_quad quad;

  stbtt_GetPackedQuad(font.charInfo.get(), font.atlasWidth, font.atlasHeight,
                      character - font.firstChar, &offsetX, &offsetY, &quad, 1);
  auto xmin = quad.x0;
  auto xmax = quad.x1;
  auto ymin = -quad.y1;
  auto ymax = -quad.y0;

  GlyphInfo info;
  info.offsetX = offsetX;
  info.offsetY = offsetY;
  info.positions[0] = {xmin, ymin, 0};
  info.positions[1] = {xmin, ymax, 0};
  info.positions[2] = {xmax, ymax, 0};
  info.positions[3] = {xmax, ymin, 0};
  info.uvs[0] = {quad.s0, quad.t1};
  info.uvs[1] = {quad.s0, quad.t0};
  info.uvs[2] = {quad.s1, quad.t0};
  info.uvs[3] = {quad.s1, quad.t1};

  return info;
}

void TrueTypeScene::initLabel() {
  mLabelCamera = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 20.0f));
  mLabelCamera->lookAt({0.0f, 0.0f, 0.0f});

  mLabelShader = std::make_unique<DL::Shader>(
      "Shaders/label.vert", "Shaders/label.frag", mGlslVersionString);

  const std::string text = "GRuwL/Sector90";

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<uint16_t> indexes;

  uint16_t lastIndex = 0;
  float offsetX = 0, offsetY = 0;
  for (auto c : text) {
    const auto glyphInfo = makeGlyphInfo(c, offsetX, offsetY);
    offsetX = glyphInfo.offsetX;
    offsetY = glyphInfo.offsetY;

    vertices.emplace_back(glyphInfo.positions[0]);
    vertices.emplace_back(glyphInfo.positions[1]);
    vertices.emplace_back(glyphInfo.positions[2]);
    vertices.emplace_back(glyphInfo.positions[3]);
    uvs.emplace_back(glyphInfo.uvs[0]);
    uvs.emplace_back(glyphInfo.uvs[1]);
    uvs.emplace_back(glyphInfo.uvs[2]);
    uvs.emplace_back(glyphInfo.uvs[3]);
    indexes.push_back(lastIndex);
    indexes.push_back(lastIndex + 1);
    indexes.push_back(lastIndex + 2);
    indexes.push_back(lastIndex);
    indexes.push_back(lastIndex + 2);
    indexes.push_back(lastIndex + 3);

    lastIndex += 4;
  }

  glGenVertexArrays(1, &rotatingLabel.vao);
  glBindVertexArray(rotatingLabel.vao);

  glGenBuffers(1, &rotatingLabel.vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, rotatingLabel.vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);

  glGenBuffers(1, &rotatingLabel.uvBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, rotatingLabel.uvBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * uvs.size(), uvs.data(),
               GL_STATIC_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  rotatingLabel.indexElementCount = indexes.size();
  glGenBuffers(1, &rotatingLabel.indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rotatingLabel.indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               sizeof(uint16_t) * rotatingLabel.indexElementCount,
               indexes.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void TrueTypeScene::renderLabel(float delta) {

  mLabelShader->use();

  glm::vec3 pivot = {260, 0, 0};
  glm::mat4 trans_to_pivot = glm::translate(glm::mat4(1.0f), -pivot);
  glm::mat4 trans_from_pivot = glm::translate(glm::mat4(1.0f), pivot);

  glm::mat4 rotate_matrix =
      glm::rotate(glm::mat4(1.0f), glm::radians<float>(glfwGetTime() * 50),
                  glm::vec3(0.0, 1.0, 0.0));

  glm::mat4 rotate = trans_from_pivot * rotate_matrix * trans_to_pivot;
  glm::mat4 scale_matrix =
      glm::scale(glm::mat4(1.0f), glm::vec3(0.05, 0.05, 0.05));
  rotate = scale_matrix * trans_to_pivot * rotate;
  mLabelShader->setMat4f("model", rotate);

  glm::mat4 view = mLabelCamera->getViewMatrix();
  mLabelShader->setMat4f("view", view);

  glm::mat4 projectionMatrix = mLabelCamera->getPerspectiveTransform(
      45.0, mScreenSize.x / mScreenSize.y);
  mLabelShader->setMat4f("projection", projectionMatrix);

  glBindVertexArray(rotatingLabel.vao);

  mLabelShader->setFloat("iTime", glfwGetTime());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, font.texture);
  mLabelShader->setInt("texture1", 0);

  glBindVertexArray(rotatingLabel.vao);
  glDrawElements(GL_TRIANGLES, rotatingLabel.indexElementCount,
                 GL_UNSIGNED_SHORT, nullptr);
}

void TrueTypeScene::init() {
  glEnable(GL_DEPTH_TEST);
  loadFontTexture();
  initLabel();
}

void TrueTypeScene::render(float delta) {
  mDelta = delta;

  glClearColor(0.5, 0.5, 0.5, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  renderLabel(delta);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("true type demo");
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Image((void *)font.texture, {512.0, 512.0});
  ImGui::End();
}

void TrueTypeScene::onKey(int key){};

void TrueTypeScene::onScreenSizeChanged(glm::vec2 size) { mScreenSize = size; };
