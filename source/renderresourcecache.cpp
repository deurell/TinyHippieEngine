#include "renderresourcecache.h"

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
  if (whiteTexture_.valid()) {
    renderDevice_.destroy(whiteTexture_);
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

} // namespace DL
