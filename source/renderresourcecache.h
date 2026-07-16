#pragma once

#include "basisu_global_selector_palette.h"
#include "renderdevice.h"
#include "stb_truetype.h"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace DL {

struct FontAtlasResource {
  TextureHandle texture;
  std::shared_ptr<stbtt_packedchar[]> fontInfo;
  float fontScale = 1.0f;
  float fontSize = 0.0f;
  std::uint32_t atlasWidth = 0;
  std::uint32_t atlasHeight = 0;
};

struct ImageTextureResource {
  TextureHandle texture;
  glm::vec2 size{1.0f, 1.0f};
};

class RenderResourceCache {
public:
  explicit RenderResourceCache(IRenderDevice &renderDevice)
      : renderDevice_(renderDevice) {}
  ~RenderResourceCache();

  PipelineHandle acquirePipeline(std::string_view vertexPath,
                                 std::string_view fragmentPath);
  TextureHandle acquireBasisTexture(
      std::string_view path,
      basist::etc1_global_selector_codebook &codebook);
  const ImageTextureResource *acquireImageTexture(std::string_view path);
  TextureHandle acquireWhiteTexture();
  MeshHandle acquireTexturedQuad();
  const FontAtlasResource *acquireFontAtlas(std::string_view path,
                                            float pixelHeight,
                                            std::uint32_t atlasWidth,
                                            std::uint32_t atlasHeight,
                                            std::uint32_t oversampleX,
                                            std::uint32_t oversampleY,
                                            const std::vector<int> &codepoints);

private:
  IRenderDevice &renderDevice_;
  std::unordered_map<std::string, PipelineHandle> pipelines_;
  std::unordered_map<std::string, TextureHandle> basisTextures_;
  std::unordered_map<std::string, ImageTextureResource> imageTextures_;
  std::unordered_map<std::string, FontAtlasResource> fontAtlases_;
  TextureHandle whiteTexture_;
  MeshHandle texturedQuad_;
};

} // namespace DL
