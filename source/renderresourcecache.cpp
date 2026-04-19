#include "renderresourcecache.h"

#include <fstream>

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
    std::uint32_t oversampleY, std::uint8_t firstChar,
    std::uint8_t charCount) {
  const std::string key = std::string(path) + "|" + std::to_string(pixelHeight) +
                          "|" + std::to_string(atlasWidth) + "|" +
                          std::to_string(atlasHeight) + "|" +
                          std::to_string(oversampleX) + "|" +
                          std::to_string(oversampleY) + "|" +
                          std::to_string(firstChar) + "|" +
                          std::to_string(charCount);
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
  resource.fontScale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

  int ascent = 0;
  int descent = 0;
  int lineGap = 0;
  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
  resource.fontSize = (ascent - descent + lineGap) * resource.fontScale;

  auto atlasData =
      std::make_unique<std::uint8_t[]>(atlasWidth * atlasHeight);
  resource.fontInfo = std::shared_ptr<stbtt_packedchar[]>(
      new stbtt_packedchar[charCount], [](stbtt_packedchar *p) { delete[] p; });

  stbtt_pack_context context;
  if (!stbtt_PackBegin(&context, atlasData.get(),
                       static_cast<int>(atlasWidth),
                       static_cast<int>(atlasHeight), 0, 1, nullptr)) {
    return nullptr;
  }
  stbtt_PackSetOversampling(&context, oversampleX, oversampleY);

  if (!stbtt_PackFontRange(
          &context, reinterpret_cast<const unsigned char *>(fontData.get()), 0,
          resource.fontSize, firstChar, charCount, resource.fontInfo.get())) {
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
