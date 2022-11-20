#include "textsprite.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <iostream>
#include <utility>

DL::TextSprite::TextSprite(std::string_view fontPath, std::wstring_view text)
    : mText(text) {
  loadFontTexture(fontPath);
  init();
}

DL::TextSprite::TextSprite(std::string_view fontPath)
    : TextSprite(fontPath, L"") {}

DL::TextSprite::TextSprite(GLuint texture, stbtt_packedchar *fontInfo,
                           std::wstring text)
    : mFontTexture(texture), mFontCharInfoPtr(fontInfo),
      mText(std::move(text)) {
  glBindTexture(GL_TEXTURE_2D, mFontTexture);
  init();
}

void DL::TextSprite::render(float /*delta*/) const {
  glBindVertexArray(mVAO);
  glDrawElements(GL_TRIANGLES, mIndexElementCount, GL_UNSIGNED_SHORT, nullptr);
}

void DL::TextSprite::loadFontTexture(std::string_view fontPath) {
  std::ifstream iStream(std::string(fontPath), std::ios::binary);
  iStream.seekg(0, iStream.end);
  const int size = iStream.tellg();
  iStream.seekg(0, iStream.beg);
  char *fontData = new char[size];

  iStream.read((char *)fontData, size);
  iStream.close();

  auto atlasData =
      std::make_unique<uint8_t[]>(mFontAtlasWidth * mFontAtlasHeight);

  mFontCharInfo = std::make_unique<stbtt_packedchar[]>(mFontCharCount);
  mFontCharInfoPtr = mFontCharInfo.get();

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(), mFontAtlasWidth,
                       mFontAtlasHeight, 0, 1, nullptr)) {
    std::cout << "init font failed.";
  }
  stbtt_PackSetOversampling(&context, mFontOversampleX, mFontOversampleY);

  if (!stbtt_PackFontRange(
          &context, reinterpret_cast<const unsigned char *>(fontData), 0,
          mFontSize, mFontFirstChar, mFontCharCount, mFontCharInfo.get())) {
    std::cout << "pack font failed";
  }

  stbtt_PackEnd(&context);
  glGenTextures(1, &mFontTexture);
  glBindTexture(GL_TEXTURE_2D, mFontTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mFontAtlasWidth, mFontAtlasHeight, 0,
               GL_RED, GL_UNSIGNED_BYTE, atlasData.get());
  glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
  glGenerateMipmap(GL_TEXTURE_2D);
  delete[] fontData;
}

stbtt_packedchar *DL::TextSprite::getFontCharInfoPtr() {
  return mFontCharInfo ? mFontCharInfo.get() : mFontCharInfoPtr;
}

void DL::TextSprite::init() {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<uint16_t> indexes;

  uint16_t lastIndex = 0;
  float offsetX = 0, offsetY = 0;
  for (auto c : mText) {
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

  glGenVertexArrays(1, &mVAO);
  glBindVertexArray(mVAO);

  glGenBuffers(1, &mVBO);
  glBindBuffer(GL_ARRAY_BUFFER, mVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(),
               vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(0);
  glGenBuffers(1, &mUVBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * uvs.size(), uvs.data(),
               GL_STATIC_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(1);

  mIndexElementCount = indexes.size();
  glGenBuffers(1, &mIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * mIndexElementCount,
               indexes.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

DL::GlyphInfo DL::TextSprite::makeGlyphInfo(uint32_t character, float offsetX,
                                            float offsetY) {
  stbtt_aligned_quad quad;
  int chrRel = static_cast<int>(character - mFontFirstChar);
  stbtt_GetPackedQuad(getFontCharInfoPtr(), mFontAtlasWidth, mFontAtlasHeight,
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
