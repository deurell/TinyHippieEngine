#include "textvisualizer.h"
#include <glm/glm.hpp>
#include <string>

DL::TextVisualizer::TextVisualizer(std::string name, DL::Camera &camera,
                                   std::string_view glslVersionString,
                                   SceneNode &node, std::string_view text)
    : VisualizerBase(camera, std::move(name), std::string(glslVersionString),
                     "Shaders/status.vert", "Shaders/status.frag", node),
      text_(text) {

  camera.lookAt({0, 0, 0});
  shader_ = std::make_unique<DL::Shader>(vertexShaderPath_, fragmentShaderPath_,
                                         glslVersionString_);

  loadFontTexture("Resources/C64_Pro-STYLE.ttf");
  initGraphics();
}

void DL::TextVisualizer::render(const glm::mat4 &worldTransform, float delta) {
  shader_->use();
  glm::mat4 model = glm::mat4(1.0f);
  auto position = extractPosition(worldTransform);
  auto rotation = extractRotation(worldTransform);
  auto scale = extractScale(worldTransform);

  model = glm::translate(model, position);
  model = model * glm::mat4_cast(rotation);
  model = glm::scale(model, glm::vec3(1.0, 1.0, 1.0) * scale);

  shader_->setMat4f("model", model);
  glm::mat4 view = camera_.getViewMatrix();
  shader_->setMat4f("view", view);
  glm::mat4 projectionMatrix = camera_.getPerspectiveTransform();
  shader_->setMat4f("projection", projectionMatrix);

  shader_->setFloat("iTime", static_cast<float>(glfwGetTime()));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, fontTexture_);
  shader_->setInt("texture1", 0);

  glBindVertexArray(VAO_);
  glDrawElements(GL_TRIANGLES, indexElementCount_, GL_UNSIGNED_SHORT, nullptr);
}

void DL::TextVisualizer::loadFontTexture(std::string_view fontPath) {
  std::ifstream iStream(std::string(fontPath), std::ios::binary);
  iStream.seekg(0, iStream.end);
  const int size = iStream.tellg();
  iStream.seekg(0, iStream.beg);
  char *fontData = new char[size];

  iStream.read((char *)fontData, size);
  iStream.close();

   stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, (unsigned char*)fontData, 0)) {
        std::cout << "Failed to initialize font info." << std::endl;
        return;
    }
    
    // Determine the scale for the desired pixel height
    fontScale_ = stbtt_ScaleForPixelHeight(&fontInfo, desiredPixelHeight_);

     int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    // Convert to actual height using the scale.
    float actualFontHeight = (ascent - descent + lineGap) * fontScale_;

    // You can then use this actualFontHeight as your equivalent 'font size'
    // when packing the atlas. Note that it's an approximation.
    fontSize_ = actualFontHeight; // Update mFontSize with this value.


  auto atlasData =
      std::make_unique<uint8_t[]>(fontAtlasWidth_ * FontAtlasHeight_);

  fontCharInfo_ = std::make_unique<stbtt_packedchar[]>(fontCharCount_);
  fontCharInfoPtr_ = fontCharInfo_.get();

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(), fontAtlasWidth_,
                       FontAtlasHeight_, 0, 1, nullptr)) {
    std::cout << "init font failed.";
  }
  stbtt_PackSetOversampling(&context, fontOversampleX_, fontOversampleY_);

  if (!stbtt_PackFontRange(
          &context, reinterpret_cast<const unsigned char *>(fontData), 0,
          fontSize_, fontFirstChar_, fontCharCount_, fontCharInfo_.get())) {
    std::cout << "pack font failed";
  }

  stbtt_PackEnd(&context);
  glGenTextures(1, &fontTexture_);
  glBindTexture(GL_TEXTURE_2D, fontTexture_);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, fontAtlasWidth_, FontAtlasHeight_, 0,
               GL_RED, GL_UNSIGNED_BYTE, atlasData.get());
  glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
  glGenerateMipmap(GL_TEXTURE_2D);
  delete[] fontData;
}

stbtt_packedchar *DL::TextVisualizer::getFontCharInfoPtr() {
  return fontCharInfo_ ? fontCharInfo_.get() : fontCharInfoPtr_;
}

DL::GlyphInfo DL::TextVisualizer::makeGlyphInfo(char character, float offsetX,
                                                float offsetY) {
  stbtt_aligned_quad quad;
  int chrRel = static_cast<uint8_t>(character - fontFirstChar_);
  stbtt_GetPackedQuad(getFontCharInfoPtr(), fontAtlasWidth_, FontAtlasHeight_,
                      chrRel, &offsetX, &offsetY, &quad, 1);

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

void DL::TextVisualizer::initGraphics() {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<uint16_t> indexes;

  uint16_t lastIndex = 0;
  float offsetX = 0, offsetY = 0;
  for (auto c : text_) {
    if (c == '\n') {
      offsetY += 42;
      offsetX = 0;
      continue;
    }

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

  glGenVertexArrays(1, &VAO_);
  glBindVertexArray(VAO_);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);
  glGenBuffers(1, &UVBuffer_);
  glBindBuffer(GL_ARRAY_BUFFER, UVBuffer_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * uvs.size(), uvs.data(),
               GL_STATIC_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  indexElementCount_ = indexes.size();
  glGenBuffers(1, &indexBuffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indexElementCount_,
               indexes.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}