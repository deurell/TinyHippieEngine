#include "textvisualizer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::map<std::string, DL::FontData> fontCache_;

DL::TextVisualizer::TextVisualizer(std::string name, DL::Camera &camera,
                                   SceneNode &node, const std::string text,
                                   const std::string &fontPath,
                                   DL::IRenderDevice *renderDevice,
                                   const std::string vertexShaderPath,
                                   const std::string fragmentShaderPath)
    : VisualizerBase(camera, name, vertexShaderPath, fragmentShaderPath, node),
      text_(text), renderDevice_(renderDevice), fontPath_(fontPath) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = renderDevice_->createPipeline(vertexShaderPath_, fragmentShaderPath_);

  auto it = fontCache_.find(fontCacheKey());
  if (it != fontCache_.end()) {
    it->second.refCount++;
    fontTexture_ = it->second.texture;
    fontCharInfo_ = it->second.fontInfo;
    fontScale_ = it->second.fontScale;
    fontSize_ = it->second.fontSize;
  } else {
    if (loadFontTexture(fontPath_)) {
      FontData data{};
      data.texture = fontTexture_;
      data.fontInfo = fontCharInfo_;
      data.fontScale = fontScale_;
      data.fontSize = fontSize_;
      data.refCount = 1;
      fontCache_.insert(std::make_pair(fontCacheKey(), std::move(data)));
    } else {
      return;
    }
  }

  initGraphics();
}

DL::TextVisualizer::~TextVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid()) {
      renderDevice_->destroy(pipeline_);
    }
  }
  releaseFont();
}

void DL::TextVisualizer::render(const glm::mat4 &worldTransform,
                                const DL::FrameContext &ctx) {
  if (renderDevice_ == nullptr || !mesh_.valid() || !fontTexture_.valid() ||
      !pipeline_.valid()) {
    return;
  }

  glm::mat4 model =
      glm::translate(glm::mat4(1.0f), extractPosition(worldTransform));
  model *= glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  glm::mat4 viewMatrix = camera_.getViewMatrix();
  glm::mat4 perspectiveTransform = camera_.getPerspectiveTransform();

  DrawCommand command;
  command.mesh = mesh_;
  command.pipeline = pipeline_;
  command.texture = fontTexture_;
  command.uniforms.push_back(
      UniformValue::makeMat4("model", model));
  command.uniforms.push_back(
      UniformValue::makeMat4("view", viewMatrix));
  command.uniforms.push_back(
      UniformValue::makeMat4("projection", perspectiveTransform));
  command.uniforms.push_back(
      UniformValue::makeFloat("iTime", static_cast<float>(ctx.total_time)));
  command.uniforms.push_back(UniformValue::makeFloat("rotAngle1", rotAngle1_));
  command.uniforms.push_back(UniformValue::makeFloat("rotAngle2", rotAngle2_));
  command.uniforms.push_back(UniformValue::makeFloat("c1", color1_));
  command.uniforms.push_back(UniformValue::makeFloat("c2", color2_));
  renderDevice_->draw(command);
}

void DL::TextVisualizer::setText(std::string text) {
  text_ = std::move(text);
  initGraphics();
}

void DL::TextVisualizer::setAlignment(TextAlignment alignment) {
  if (alignment_ == alignment) {
    return;
  }
  alignment_ = alignment;
  initGraphics();
}

void DL::TextVisualizer::setLayoutWidth(float width) {
  if (layoutWidth_ == width) {
    return;
  }
  layoutWidth_ = width;
  initGraphics();
}

bool DL::TextVisualizer::loadFontTexture(std::string_view fontPath) {
  std::ifstream iStream(std::string(fontPath), std::ios::binary);
  if (!iStream) {
    std::cout << "Failed to open font: " << fontPath << std::endl;
    return false;
  }
  iStream.seekg(0, std::ifstream::end);
  const std::streamoff size = iStream.tellg();
  if (size <= 0) {
    std::cout << "Failed to read font: " << fontPath << std::endl;
    return false;
  }
  iStream.seekg(0, std::ifstream::beg);

  std::unique_ptr<char[]> fontData =
      std::make_unique<char[]>(static_cast<size_t>(size));
  iStream.read(fontData.get(), size);
  if (!iStream) {
    std::cout << "Failed to load font data: " << fontPath << std::endl;
    return false;
  }
  iStream.close();

  stbtt_fontinfo fontInfo;
  if (!stbtt_InitFont(&fontInfo, (unsigned char *)fontData.get(), 0)) {
    std::cout << "Failed to initialize font info." << std::endl;
    return false;
  }

  fontScale_ = stbtt_ScaleForPixelHeight(&fontInfo, desiredPixelHeight_);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

  float actualFontHeight = (ascent - descent + lineGap) * fontScale_;

  fontSize_ = actualFontHeight;

  auto atlasData =
      std::make_unique<uint8_t[]>(fontAtlasWidth_ * FontAtlasHeight_);

  fontCharInfo_ = std::shared_ptr<stbtt_packedchar[]>(
      new stbtt_packedchar[fontCharCount_],
      [](stbtt_packedchar *p) { delete[] p; });

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(), fontAtlasWidth_,
                       FontAtlasHeight_, 0, 1, nullptr)) {
    std::cout << "init font failed." << std::endl;
    fontCharInfo_.reset();
    return false;
  }
  stbtt_PackSetOversampling(&context, fontOversampleX_, fontOversampleY_);

  if (!stbtt_PackFontRange(
          &context, reinterpret_cast<const unsigned char *>(fontData.get()), 0,
          fontSize_, fontFirstChar_, fontCharCount_, fontCharInfo_.get())) {
    std::cout << "pack font failed" << std::endl;
    stbtt_PackEnd(&context);
    fontCharInfo_.reset();
    return false;
  }

  stbtt_PackEnd(&context);
  if (renderDevice_ != nullptr) {
    fontTexture_ = renderDevice_->createTexture({
        .pixels = atlasData.get(),
        .width = fontAtlasWidth_,
        .height = FontAtlasHeight_,
        .format = TextureFormat::R8,
        .generateMipmaps = true,
    });
  }
  return fontTexture_.valid();
}

DL::TextGlyphInfo DL::TextVisualizer::makeGlyphInfo(char character,
                                                    float offsetX,
                                                    float offsetY) {
  stbtt_aligned_quad quad;
  const uint8_t glyph = static_cast<uint8_t>(character);
  const uint8_t firstChar = fontFirstChar_;
  const uint8_t lastChar = static_cast<uint8_t>(fontFirstChar_ + fontCharCount_);
  const uint8_t clampedGlyph =
      glyph >= firstChar && glyph < lastChar ? glyph : static_cast<uint8_t>('?');
  int chrRel = static_cast<int>(clampedGlyph) - static_cast<int>(firstChar);
  stbtt_GetPackedQuad(fontCharInfo_.get(), fontAtlasWidth_, FontAtlasHeight_,
                      chrRel, &offsetX, &offsetY, &quad, 1);

  auto [xmin, xmax] = std::minmax({quad.x0, quad.x1});
  auto [ymin, ymax] = std::minmax({-quad.y1, -quad.y0});

  return TextGlyphInfo{
      {{xmin, ymin, 0}, {xmin, ymax, 0}, {xmax, ymax, 0}, {xmax, ymin, 0}},
      {{quad.s0, quad.t1},
       {quad.s0, quad.t0},
       {quad.s1, quad.t0},
       {quad.s1, quad.t1}},
      offsetX,
      offsetY};
}

void DL::TextVisualizer::initGraphics() {
  if (renderDevice_ == nullptr || !fontTexture_.valid() || !fontCharInfo_) {
    return;
  }

  destroyMesh();

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<uint16_t> indices;

  uint16_t index = 0;
  glm::vec2 offset(0.0f, 0.0f);

  std::vector<std::string> lines;
  std::stringstream ss(text_);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  const float viewportWidth =
      layoutWidth_ > 0.0f ? layoutWidth_ : camera_.mScreenSize.x;

  // Calculate the width of a space character
  float spaceWidth = (makeGlyphInfo('A', 0.0f, 0.0f).positions[2].x -
                      makeGlyphInfo('A', 0.0f, 0.0f).positions[0].x);
  spaceWidth /= 2;
  for (const auto &line : lines) {
    if (alignment_ == TextAlignment::CENTER) {
      float totalLineWidth = 0.0f;
      for (char c : line) {
        TextGlyphInfo glyphInfo = makeGlyphInfo(c, 0.0f, 0.0f);
        totalLineWidth +=
            (glyphInfo.positions[2].x - glyphInfo.positions[0].x) +
            kerning_; // Include kerning in the width
      }
      totalLineWidth -= kerning_; // Remove the last kerning
      offset.x = viewportWidth > 0.0f ? (viewportWidth - totalLineWidth) * 0.5f
                                      : totalLineWidth * -0.5f;
    }

    for (char c : line) {
      TextGlyphInfo glyphInfo = makeGlyphInfo(c, offset.x, offset.y);
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

      // Adjust the offset.x for every character, including spaces
      if (c == ' ') {
        offset.x += spaceWidth;
      } else {
        offset.x += (glyphInfo.positions[2].x - glyphInfo.positions[0].x);
      }
      offset.x += 2.0; // Kerning
    }

    offset.y +=
        fontSize_ + kerning_; // Use the calculated font size for line height
    offset.x = 0.0f;          // Reset the X offset for the next line
  }

  if (renderDevice_ != nullptr) {
    mesh_ = renderDevice_->createMesh(vertices, uvs, indices);
  }
}

void DL::TextVisualizer::destroyMesh() {
  if (renderDevice_ != nullptr && mesh_.valid()) {
    renderDevice_->destroy(mesh_);
    mesh_ = {};
  }
}

void DL::TextVisualizer::releaseFont() {
  if (fontPath_.empty() || renderDevice_ == nullptr) {
    return;
  }
  auto it = fontCache_.find(fontCacheKey());
  if (it != fontCache_.end()) {
    if (it->second.refCount > 0) {
      it->second.refCount--;
    }
    if (it->second.refCount == 0) {
      if (it->second.texture.valid()) {
        renderDevice_->destroy(it->second.texture);
      }
      fontCache_.erase(it);
    }
  }
  fontTexture_ = {};
  fontCharInfo_.reset();
  fontPath_.clear();
}

std::string DL::TextVisualizer::fontCacheKey() const {
  return fontPath_ + "@" +
         std::to_string(reinterpret_cast<std::uintptr_t>(renderDevice_));
}
