#include "textvisualizer.h"
#include "renderqueue.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

namespace {

const std::vector<int> &textGlyphCodepoints() {
  static const std::vector<int> codepoints = [] {
    std::vector<int> result;
    for (int codepoint = 32; codepoint < 127; ++codepoint) {
      result.push_back(codepoint);
    }
    result.push_back(0x00C5); // Å
    result.push_back(0x00C4); // Ä
    result.push_back(0x00D6); // Ö
    result.push_back(0x00E5); // å
    result.push_back(0x00E4); // ä
    result.push_back(0x00F6); // ö
    return result;
  }();
  return codepoints;
}

int glyphIndexForCodepoint(std::uint32_t codepoint) {
  const auto &codepoints = textGlyphCodepoints();
  const auto it = std::find(codepoints.begin(), codepoints.end(),
                            static_cast<int>(codepoint));
  if (it != codepoints.end()) {
    return static_cast<int>(std::distance(codepoints.begin(), it));
  }
  const auto fallback = std::find(codepoints.begin(), codepoints.end(),
                                  static_cast<int>('?'));
  return static_cast<int>(std::distance(codepoints.begin(), fallback));
}

std::vector<std::uint32_t> decodeUtf8(std::string_view text) {
  std::vector<std::uint32_t> result;
  for (std::size_t i = 0; i < text.size();) {
    const auto c = static_cast<unsigned char>(text[i]);
    if (c < 0x80) {
      result.push_back(c);
      ++i;
      continue;
    }

    if ((c & 0xE0u) == 0xC0u && i + 1 < text.size()) {
      const auto c1 = static_cast<unsigned char>(text[i + 1]);
      if ((c1 & 0xC0u) == 0x80u) {
        result.push_back(((c & 0x1Fu) << 6u) | (c1 & 0x3Fu));
        i += 2;
        continue;
      }
    }

    if ((c & 0xF0u) == 0xE0u && i + 2 < text.size()) {
      const auto c1 = static_cast<unsigned char>(text[i + 1]);
      const auto c2 = static_cast<unsigned char>(text[i + 2]);
      if ((c1 & 0xC0u) == 0x80u && (c2 & 0xC0u) == 0x80u) {
        result.push_back(((c & 0x0Fu) << 12u) | ((c1 & 0x3Fu) << 6u) |
                         (c2 & 0x3Fu));
        i += 3;
        continue;
      }
    }

    result.push_back('?');
    ++i;
  }
  return result;
}

} // namespace

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

  if (!acquireFontTexture()) {
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

  auto makeItem = [&](const glm::mat4 &drawModel, glm::vec4 color) {
    RenderItem item;
    item.tag = RenderTag::Text;
    item.localBounds = localBounds_;
    item.mesh = mesh_;
    item.pipeline = pipeline_;
    item.texture = fontTexture_;
    item.pass = pass;
    item.blendMode = BlendMode::Alpha;
    item.depthTest = false;
    item.sortMode = DrawSortMode::BackToFront;
    item.sortDepth = sortDepth;
    item.uniforms.push_back(UniformValue::makeMat4("model", drawModel));
    item.uniforms.push_back(UniformValue::makeMat4("view", viewMatrix));
    item.uniforms.push_back(
        UniformValue::makeMat4("projection", perspectiveTransform));
    item.uniforms.push_back(UniformValue::makeVec4("textColor", color));
    return item;
  };

  auto submit = [&](RenderItem item) {
    submitRenderItem(ctx, *renderDevice_, std::move(item));
  };

  if (shadowColor_.a > 0.0f &&
      (shadowOffset_.x != 0.0f || shadowOffset_.y != 0.0f)) {
    const glm::mat4 shadowModel =
        model * glm::translate(glm::mat4(1.0f),
                               glm::vec3(shadowOffset_.x, shadowOffset_.y,
                                         0.0f));
    submit(makeItem(shadowModel, shadowColor_));
  }

  submit(makeItem(model, textColor_));
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

void DL::TextVisualizer::setFontPixelHeight(float pixelHeight) {
  pixelHeight = std::max(pixelHeight, 1.0f);
  if (desiredPixelHeight_ == pixelHeight) {
    return;
  }

  const float previousPixelHeight = desiredPixelHeight_;
  desiredPixelHeight_ = pixelHeight;

  if (resourceCache_ != nullptr) {
    if (!acquireFontTexture()) {
      desiredPixelHeight_ = previousPixelHeight;
      return;
    }
    initGraphics();
    return;
  }

  destroyMesh();
  if (fontTexture_.valid() && !sharedFontTexture_ && renderDevice_ != nullptr) {
    renderDevice_->destroy(fontTexture_);
  }
  fontTexture_ = {};
  fontCharInfo_.reset();
  sharedFontTexture_ = false;

  if (!acquireFontTexture()) {
    desiredPixelHeight_ = previousPixelHeight;
    return;
  }
  initGraphics();
}

bool DL::TextVisualizer::acquireFontTexture() {
  const std::uint32_t atlasSize = atlasSizeForPixelHeight();
  if (resourceCache_ != nullptr) {
    const auto *fontAtlas = resourceCache_->acquireFontAtlas(
        fontPath_, desiredPixelHeight_, atlasSize, atlasSize,
        fontOversampleX_, fontOversampleY_, textGlyphCodepoints());
    if (fontAtlas == nullptr) {
      return false;
    }
    fontTexture_ = fontAtlas->texture;
    fontCharInfo_ = fontAtlas->fontInfo;
    fontScale_ = fontAtlas->fontScale;
    fontSize_ = fontAtlas->fontSize;
    fontAtlasWidth_ = fontAtlas->atlasWidth;
    fontAtlasHeight_ = fontAtlas->atlasHeight;
    sharedFontTexture_ = true;
    return true;
  }

  return loadFontTexture(fontPath_);
}

std::uint32_t DL::TextVisualizer::atlasSizeForPixelHeight() const {
  if (desiredPixelHeight_ <= 64.0f) {
    return 1024;
  }
  if (desiredPixelHeight_ <= 128.0f) {
    return 2048;
  }
  return 4096;
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
  fontAtlasWidth_ = atlasSizeForPixelHeight();
  fontAtlasHeight_ = fontAtlasWidth_;

  auto atlasData =
      std::make_unique<uint8_t[]>(fontAtlasWidth_ * fontAtlasHeight_);

  fontCharInfo_ = std::shared_ptr<stbtt_packedchar[]>(
      new stbtt_packedchar[textGlyphCodepoints().size()],
      [](stbtt_packedchar *p) { delete[] p; });

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(), fontAtlasWidth_,
                       fontAtlasHeight_, 0, 1, nullptr)) {
    std::cout << "init font failed." << std::endl;
    fontCharInfo_.reset();
    return false;
  }
  stbtt_PackSetOversampling(&context, fontOversampleX_, fontOversampleY_);

  stbtt_pack_range range{};
  range.font_size = fontSize_;
  range.first_unicode_codepoint_in_range = 0;
  range.array_of_unicode_codepoints =
      const_cast<int *>(textGlyphCodepoints().data());
  range.num_chars = static_cast<int>(textGlyphCodepoints().size());
  range.chardata_for_range = fontCharInfo_.get();
  if (!stbtt_PackFontRanges(
          &context, reinterpret_cast<const unsigned char *>(fontData.get()), 0,
          &range, 1)) {
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
        .height = fontAtlasHeight_,
        .format = TextureFormat::R8,
        .generateMipmaps = true,
    });
  }
  return fontTexture_.valid();
}

DL::TextGlyphInfo DL::TextVisualizer::makeGlyphInfo(std::uint32_t codepoint,
                                                    float offsetX,
                                                    float offsetY) {
  stbtt_aligned_quad quad;
  const int chrRel = glyphIndexForCodepoint(codepoint);
  stbtt_GetPackedQuad(fontCharInfo_.get(), fontAtlasWidth_, fontAtlasHeight_,
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

  std::vector<std::vector<std::uint32_t>> lines;
  std::stringstream ss(text_);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(decodeUtf8(line));
  }

  const float viewportWidth = layoutWidth_;

  float spaceWidth = (makeGlyphInfo('A', 0.0f, 0.0f).positions[2].x -
                      makeGlyphInfo('A', 0.0f, 0.0f).positions[0].x);
  spaceWidth /= 2;
  for (const auto &line : lines) {
    float totalLineWidth = 0.0f;
    for (std::uint32_t c : line) {
      if (c == static_cast<std::uint32_t>(' ')) {
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

    for (std::uint32_t c : line) {
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
      if (c == static_cast<std::uint32_t>(' ')) {
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

    glm::vec3 minLocal(std::numeric_limits<float>::max());
    glm::vec3 maxLocal(std::numeric_limits<float>::lowest());
    for (const glm::vec3 &vertex : vertices) {
      minLocal = glm::min(minLocal, vertex);
      maxLocal = glm::max(maxLocal, vertex);
    }
    localBounds_.center = (minLocal + maxLocal) * 0.5f;
    localBounds_.halfExtents = (maxLocal - minLocal) * 0.5f;
  } else {
    localBounds_ = {};
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
