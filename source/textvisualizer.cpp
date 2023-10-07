#include "textvisualizer.h"
#include <glm/glm.hpp>
#include <string>

DL::TextVisualizer::TextVisualizer(std::string name, DL::Camera &camera,
                                   std::string_view glslVersionString,
                                   SceneNode &node, std::string_view text)
    : VisualizerBase(camera, std::move(name), std::string(glslVersionString),
                     "Shaders/status.vert", "Shaders/status.frag", node),
      text_(text),
      shader_(std::make_unique<DL::Shader>(vertexShaderPath_, fragmentShaderPath_, glslVersionString_)) {

  camera.lookAt({0, 0, 0});
  loadFontTexture("Resources/C64_Pro-STYLE.ttf");
  initGraphics();
}

void DL::TextVisualizer::render(const glm::mat4 &worldTransform, float delta) {
  shader_->use();

  glm::mat4 model = glm::translate(glm::mat4(1.0f), extractPosition(worldTransform));
  model *= glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  glm::mat4 viewMatrix = camera_.getViewMatrix();
  glm::mat4 perspectiveTransform = camera_.getPerspectiveTransform();
  shader_->setMat4f("model", model);
  shader_->setMat4f("view", viewMatrix);
  shader_->setMat4f("projection", perspectiveTransform);
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

  std::unique_ptr<char[]> fontData = std::make_unique<char[]>(size);
  iStream.read(fontData.get(), size);
  iStream.close();

  stbtt_fontinfo fontInfo;
  if (!stbtt_InitFont(&fontInfo, (unsigned char *)fontData.get(), 0)) {
    std::cout << "Failed to initialize font info." << std::endl;
    return;
  }

  fontScale_ = stbtt_ScaleForPixelHeight(&fontInfo, desiredPixelHeight_);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

  float actualFontHeight = (ascent - descent + lineGap) * fontScale_;

  fontSize_ = actualFontHeight;

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
          &context, reinterpret_cast<const unsigned char *>(fontData.get()), 0,
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

  auto [xmin, xmax] = std::minmax({quad.x0, quad.x1});
  auto [ymin, ymax] = std::minmax({-quad.y1, -quad.y0});

  return GlyphInfo{
      {{xmin, ymin, 0}, {xmin, ymax, 0}, {xmax, ymax, 0}, {xmax, ymin, 0}},
      {{quad.s0, quad.t1},
       {quad.s0, quad.t0},
       {quad.s1, quad.t0},
       {quad.s1, quad.t1}},
      offsetX,
      offsetY};
}

void DL::TextVisualizer::initGraphics() {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<uint16_t> indices;

  uint16_t index = 0;
  glm::vec2 offset(0.0f);
  for (char c : text_) {
    if (c == '\n') {
      offset.y += fontSize_;
      offset.x = 0.0f;
      continue;
    }

    GlyphInfo glyphInfo = makeGlyphInfo(c, offset.x, offset.y);
    offset.x = glyphInfo.offsetX;
    offset.y = glyphInfo.offsetY;

    for (int i = 0; i < 4; i++) {
      vertices.push_back(glyphInfo.positions[i]);
      uvs.push_back(glyphInfo.uvs[i]);
    }

    indices.push_back(index);
    indices.push_back(index + 1);
    indices.push_back(index + 2);
    indices.push_back(index);
    indices.push_back(index + 2);
    indices.push_back(index + 3);

    index += 4;
  }

  glGenVertexArrays(1, &VAO_);
  glBindVertexArray(VAO_);

  glGenBuffers(1, &VBO_);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_);
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

  indexElementCount_ = indices.size();
  glGenBuffers(1, &indexBuffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indexElementCount_,
               indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}