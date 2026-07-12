#include "textvisualizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

DL::TextVisualizer::TextVisualizer(DL::Camera &camera, SceneNode &node,
                                   const std::string text,
                                   const std::string &fontPath,
                                   DL::IRenderDevice *renderDevice,
                                   DL::RenderResourceCache *resourceCache,
                                   const std::string vertexShaderPath,
                                   const std::string fragmentShaderPath,
                                   float pixelHeight)
    : VisualizerBase(camera, vertexShaderPath, fragmentShaderPath, node),
      text_(text), renderDevice_(renderDevice), resourceCache_(resourceCache),
      desiredPixelHeight_(pixelHeight), fontPath_(fontPath) {
  if (renderDevice_ == nullptr) {
    return;
  }

  pipeline_ = resourceCache_ != nullptr
                  ? resourceCache_->acquirePipeline(vertexShaderPath_,
                                                    fragmentShaderPath_)
                  : renderDevice_->createPipeline(vertexShaderPath_,
                                                  fragmentShaderPath_);

  if (resourceCache_ != nullptr) {
    if (const auto *fontAtlas = resourceCache_->acquireFontAtlas(
            fontPath_, desiredPixelHeight_, fontAtlasWidth_, FontAtlasHeight_,
            fontOversampleX_, fontOversampleY_, fontFirstChar_,
            fontCharCount_)) {
      fontTexture_ = fontAtlas->texture;
      fontCharInfo_ = fontAtlas->fontInfo;
      fontScale_ = fontAtlas->fontScale;
      fontSize_ = fontAtlas->fontSize;
      sharedFontTexture_ = true;
    } else {
      return;
    }
  } else if (!loadFontTexture(fontPath_)) {
    return;
  }

  initGraphics();
}

DL::TextVisualizer::~TextVisualizer() {
  if (renderDevice_ != nullptr) {
    if (mesh_.valid()) {
      renderDevice_->destroy(mesh_);
    }
    if (pipeline_.valid() && resourceCache_ == nullptr) {
      renderDevice_->destroy(pipeline_);
    }
  }
  if (fontTexture_.valid() && !sharedFontTexture_ && renderDevice_ != nullptr) {
    renderDevice_->destroy(fontTexture_);
  }
}

void DL::TextVisualizer::render(const glm::mat4 &worldTransform,
                                const DL::FrameContext &ctx,
                                DL::RenderPassId pass) {
  if (pass != DL::RenderPassId::Opaque || renderDevice_ == nullptr ||
      !mesh_.valid() || !fontTexture_.valid() || !pipeline_.valid()) {
    return;
  }

  glm::mat4 model =
      glm::translate(glm::mat4(1.0f), extractPosition(worldTransform));
  model *= glm::mat4_cast(extractRotation(worldTransform));
  model = glm::scale(model, extractScale(worldTransform));

  glm::mat4 viewMatrix = camera_.getViewMatrix();
  glm::mat4 perspectiveTransform = camera_.getPerspectiveTransform();
  const glm::vec3 textPosition = extractPosition(worldTransform);
  const float sortDepth = cameraDistanceSortDepth(textPosition);

  auto makeCommand = [&](const glm::mat4 &drawModel, glm::vec4 color) {
    DrawCommand command;
    command.mesh = mesh_;
    command.pipeline = pipeline_;
    command.texture = fontTexture_;
    command.pass = pass;
    command.blendMode = BlendMode::Alpha;
    command.depthTest = false;
    command.sortMode = DrawSortMode::BackToFront;
    command.sortDepth = sortDepth;
    command.uniforms.push_back(UniformValue::makeMat4("model", drawModel));
    command.uniforms.push_back(UniformValue::makeMat4("view", viewMatrix));
    command.uniforms.push_back(
        UniformValue::makeMat4("projection", perspectiveTransform));
    command.uniforms.push_back(UniformValue::makeVec4("textColor", color));
    return command;
  };

  if (shadowColor_.a > 0.0f &&
      (shadowOffset_.x != 0.0f || shadowOffset_.y != 0.0f)) {
    const glm::mat4 shadowModel =
        model * glm::translate(glm::mat4(1.0f),
                               glm::vec3(shadowOffset_.x, shadowOffset_.y,
                                         0.0f));
    renderDevice_->draw(makeCommand(shadowModel, shadowColor_));
  }

  DrawCommand command = makeCommand(model, textColor_);
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

void DL::TextVisualizer::setAnchor(TextAnchor anchor) {
  if (anchor_ == anchor) {
    return;
  }
  anchor_ = anchor;
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
  std::vector<std::uint32_t> indices;

  std::uint32_t index = 0;
  glm::vec2 offset(0.0f, 0.0f);

  std::vector<std::string> lines;
  std::stringstream ss(text_);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  const float viewportWidth = layoutWidth_;

  float spaceWidth = (makeGlyphInfo('A', 0.0f, 0.0f).positions[2].x -
                      makeGlyphInfo('A', 0.0f, 0.0f).positions[0].x);
  spaceWidth /= 2;
  for (const auto &line : lines) {
    float totalLineWidth = 0.0f;
    for (char c : line) {
      if (c == ' ') {
        totalLineWidth += spaceWidth + kerning_;
      } else {
        TextGlyphInfo glyphInfo = makeGlyphInfo(c, 0.0f, 0.0f);
        totalLineWidth +=
            (glyphInfo.positions[2].x - glyphInfo.positions[0].x) + kerning_;
      }
    }
    if (!line.empty()) {
      totalLineWidth -= kerning_;
    }

    if (alignment_ == TextAlignment::CENTER) {
      offset.x = viewportWidth > 0.0f
                     ? (viewportWidth - totalLineWidth) * 0.5f
                     : totalLineWidth * -0.5f;
    } else if (alignment_ == TextAlignment::RIGHT) {
      offset.x =
          viewportWidth > 0.0f ? viewportWidth - totalLineWidth : -totalLineWidth;
    } else {
      offset.x = 0.0f;
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
      offset.x += kerning_;
    }

    offset.y +=
        fontSize_ + kerning_; // Use the calculated font size for line height
    offset.x = 0.0f;          // Reset the X offset for the next line
  }

  if (!vertices.empty()) {
    glm::vec2 minBounds(std::numeric_limits<float>::max());
    glm::vec2 maxBounds(std::numeric_limits<float>::lowest());
    for (const auto &vertex : vertices) {
      minBounds.x = std::min(minBounds.x, vertex.x);
      minBounds.y = std::min(minBounds.y, vertex.y);
      maxBounds.x = std::max(maxBounds.x, vertex.x);
      maxBounds.y = std::max(maxBounds.y, vertex.y);
    }

    const float centerX = (minBounds.x + maxBounds.x) * 0.5f;
    const float centerY = (minBounds.y + maxBounds.y) * 0.5f;
    glm::vec2 anchorOffset{centerX, centerY};
    switch (anchor_) {
    case TextAnchor::TOP_LEFT:
      anchorOffset = {minBounds.x, maxBounds.y};
      break;
    case TextAnchor::TOP_CENTER:
      anchorOffset = {centerX, maxBounds.y};
      break;
    case TextAnchor::TOP_RIGHT:
      anchorOffset = {maxBounds.x, maxBounds.y};
      break;
    case TextAnchor::CENTER_LEFT:
      anchorOffset = {minBounds.x, centerY};
      break;
    case TextAnchor::CENTER:
      anchorOffset = {centerX, centerY};
      break;
    case TextAnchor::CENTER_RIGHT:
      anchorOffset = {maxBounds.x, centerY};
      break;
    case TextAnchor::BOTTOM_LEFT:
      anchorOffset = {minBounds.x, minBounds.y};
      break;
    case TextAnchor::BOTTOM_CENTER:
      anchorOffset = {centerX, minBounds.y};
      break;
    case TextAnchor::BOTTOM_RIGHT:
      anchorOffset = {maxBounds.x, minBounds.y};
      break;
    }

    for (auto &vertex : vertices) {
      vertex.x -= anchorOffset.x;
      vertex.y -= anchorOffset.y;
    }
  }

  if (renderDevice_ != nullptr) {
    mesh_ = renderDevice_->createMesh(vertices, {}, uvs, indices);
  }
}

void DL::TextVisualizer::destroyMesh() {
  if (renderDevice_ != nullptr && mesh_.valid()) {
    renderDevice_->destroy(mesh_);
    mesh_ = {};
  }
}
