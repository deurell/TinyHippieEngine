#include "renderresourcecache.h"

#include "stb_image.h"
#include <fstream>
#include <iostream>

namespace DL {

RenderResourceCache::~RenderResourceCache() {
  for (const auto &[_, pipeline] : pipelines_) {
    if (pipeline.valid()) {
      renderDevice_.destroy(pipeline);
    }
  }
  for (const auto &[_, texture] : basisTextures_) {
    if (texture.valid()) {
      renderDevice_.destroy(texture);
    }
  }
  for (const auto &[_, texture] : imageTextures_) {
    if (texture.texture.valid()) {
      renderDevice_.destroy(texture.texture);
    }
  }
  for (const auto &[_, fontAtlas] : fontAtlases_) {
    if (fontAtlas.texture.valid()) {
      renderDevice_.destroy(fontAtlas.texture);
    }
  }
  if (whiteTexture_.valid()) {
    renderDevice_.destroy(whiteTexture_);
  }
  if (texturedQuad_.valid()) {
    renderDevice_.destroy(texturedQuad_);
  }
}

PipelineHandle RenderResourceCache::acquirePipeline(std::string_view vertexPath,
                                                    std::string_view fragmentPath) {
  const std::string key = std::string(vertexPath) + "|" + std::string(fragmentPath);
  const auto it = pipelines_.find(key);
  if (it != pipelines_.end()) {
    return it->second;
  }

  const auto handle = renderDevice_.createPipeline(vertexPath, fragmentPath);
  pipelines_.emplace(key, handle);
  return handle;
}

TextureHandle RenderResourceCache::acquireBasisTexture(
    std::string_view path, basist::etc1_global_selector_codebook &codebook) {
  const std::string key(path);
  const auto it = basisTextures_.find(key);
  if (it != basisTextures_.end()) {
    return it->second;
  }

  const auto handle = renderDevice_.createBasisTexture(path, codebook);
  basisTextures_.emplace(key, handle);
  return handle;
}

const ImageTextureResource *
RenderResourceCache::acquireImageTexture(std::string_view path) {
  const std::string key(path);
  const auto it = imageTextures_.find(key);
  if (it != imageTextures_.end()) {
    return &it->second;
  }

  stbi_set_flip_vertically_on_load(false);
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char *pixels = stbi_load(key.c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    std::cerr << "Failed to load image texture: " << key << "\n";
    if (pixels != nullptr) {
      stbi_image_free(pixels);
    }
    return nullptr;
  }

  ImageTextureResource resource;
  resource.texture = renderDevice_.createTexture(
      {.pixels = pixels,
       .width = static_cast<std::uint32_t>(width),
       .height = static_cast<std::uint32_t>(height),
       .format = TextureFormat::RGBA8,
       .filter = TextureFilter::Nearest,
       .generateMipmaps = false});
  resource.size = {static_cast<float>(width), static_cast<float>(height)};
  stbi_image_free(pixels);

  if (!resource.texture.valid()) {
    std::cerr << "Failed to create image texture: " << key << "\n";
    return nullptr;
  }

  auto [insertedIt, inserted] = imageTextures_.emplace(key, resource);
  return inserted ? &insertedIt->second : nullptr;
}

TextureHandle RenderResourceCache::acquireWhiteTexture() {
  if (whiteTexture_.valid()) {
    return whiteTexture_;
  }

  const std::uint8_t pixel[] = {255, 255, 255, 255};
  whiteTexture_ = renderDevice_.createTexture({.pixels = pixel,
                                               .width = 1,
                                               .height = 1,
                                               .format = TextureFormat::RGBA8,
                                               .generateMipmaps = false});
  return whiteTexture_;
}

MeshHandle RenderResourceCache::acquireTexturedQuad() {
  if (texturedQuad_.valid()) {
    return texturedQuad_;
  }

  texturedQuad_ = renderDevice_.createTexturedQuad();
  return texturedQuad_;
}

const FontAtlasResource *RenderResourceCache::acquireFontAtlas(
    std::string_view path, float pixelHeight, std::uint32_t atlasWidth,
    std::uint32_t atlasHeight, std::uint32_t oversampleX,
    std::uint32_t oversampleY, const std::vector<int> &codepoints) {
  std::string codepointKey;
  for (const int codepoint : codepoints) {
    codepointKey += std::to_string(codepoint);
    codepointKey.push_back(',');
  }
  const std::string key = std::string(path) + "|" + std::to_string(pixelHeight) +
                          "|" + std::to_string(atlasWidth) + "|" +
                          std::to_string(atlasHeight) + "|" +
                          std::to_string(oversampleX) + "|" +
                          std::to_string(oversampleY) + "|" + codepointKey;
  const auto it = fontAtlases_.find(key);
  if (it != fontAtlases_.end()) {
    return &it->second;
  }

  std::ifstream input(std::string(path), std::ios::binary);
  if (!input) {
    return nullptr;
  }
  input.seekg(0, std::ifstream::end);
  const std::streamoff size = input.tellg();
  if (size <= 0) {
    return nullptr;
  }
  input.seekg(0, std::ifstream::beg);

  std::unique_ptr<char[]> fontData =
      std::make_unique<char[]>(static_cast<std::size_t>(size));
  input.read(fontData.get(), size);
  if (!input) {
    return nullptr;
  }

  stbtt_fontinfo fontInfo;
  if (!stbtt_InitFont(&fontInfo,
                      reinterpret_cast<const unsigned char *>(fontData.get()),
                      0)) {
    return nullptr;
  }

  FontAtlasResource resource;
  resource.atlasWidth = atlasWidth;
  resource.atlasHeight = atlasHeight;
  resource.fontScale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

  int ascent = 0;
  int descent = 0;
  int lineGap = 0;
  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
  resource.fontSize = (ascent - descent + lineGap) * resource.fontScale;

  auto atlasData =
      std::make_unique<std::uint8_t[]>(atlasWidth * atlasHeight);
  resource.fontInfo = std::shared_ptr<stbtt_packedchar[]>(
      new stbtt_packedchar[codepoints.size()],
      [](stbtt_packedchar *p) { delete[] p; });

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(),
                       static_cast<int>(atlasWidth),
                       static_cast<int>(atlasHeight), 0, 1, nullptr)) {
    return nullptr;
  }
  stbtt_PackSetOversampling(&context, oversampleX, oversampleY);

  stbtt_pack_range range{};
  range.font_size = resource.fontSize;
  range.first_unicode_codepoint_in_range = 0;
  range.array_of_unicode_codepoints = const_cast<int *>(codepoints.data());
  range.num_chars = static_cast<int>(codepoints.size());
  range.chardata_for_range = resource.fontInfo.get();
  if (!stbtt_PackFontRanges(
          &context, reinterpret_cast<const unsigned char *>(fontData.get()), 0,
          &range, 1)) {
    stbtt_PackEnd(&context);
    return nullptr;
  }

  stbtt_PackEnd(&context);
  resource.texture = renderDevice_.createTexture({.pixels = atlasData.get(),
                                                  .width = atlasWidth,
                                                  .height = atlasHeight,
                                                  .format = TextureFormat::R8,
                                                  .generateMipmaps = true});
  if (!resource.texture.valid()) {
    return nullptr;
  }

  auto [insertedIt, inserted] = fontAtlases_.emplace(key, std::move(resource));
  return inserted ? &insertedIt->second : nullptr;
}

} // namespace DL
