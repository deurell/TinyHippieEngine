#pragma once

#include "basisu_global_selector_palette.h"
#include "renderdevice.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace DL {

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
  TextureHandle acquireWhiteTexture();

private:
  IRenderDevice &renderDevice_;
  std::unordered_map<std::string, PipelineHandle> pipelines_;
  std::unordered_map<std::string, TextureHandle> basisTextures_;
  TextureHandle whiteTexture_;
};

} // namespace DL
