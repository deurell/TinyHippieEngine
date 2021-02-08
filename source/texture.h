#pragma once

#include "basisu_transcoder.h"
#include "stb_image.h"
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace DL {
class Texture {
public:
  explicit Texture(std::string_view imagePath,
                   basist::etc1_global_selector_codebook &codeBook,
                   GLint format = GL_RGB,
                   GLint internal_format = GL_UNSIGNED_SHORT_5_6_5,
                   basist::transcoder_texture_format transcoder_format =
                       basist::transcoder_texture_format::cTFRGB565)
      : mId(0) {

    assert(hasExtension(imagePath, ".basis"));

    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    std::vector<unsigned char> dst_data;

    std::ifstream imageStream(std::string(imagePath), std::ios::binary);
    std::vector<unsigned char> buffer(
        std::istreambuf_iterator<char>(imageStream), {});

    uint32_t image_index = 0;
    uint32_t level_index = 0;

    auto transcoder = std::make_unique<basist::basisu_transcoder>(&codeBook);
    bool success = transcoder->start_transcoding(buffer.data(), buffer.size());
    basist::basis_texture_type textureType =
        transcoder->get_texture_type(buffer.data(), buffer.size());
    uint32_t imageCount =
        transcoder->get_total_images(buffer.data(), buffer.size());

    basist::basisu_image_info imageInfo{};
    transcoder->get_image_info(buffer.data(), buffer.size(), imageInfo,
                               image_index);

    basist::basisu_image_level_info levelInfo{};
    transcoder->get_image_level_info(buffer.data(), buffer.size(), levelInfo,
                                     image_index, level_index);
    uint32_t dest_size = 0;
    bool unCompressed = false;
    if (basist::basis_transcoder_format_is_uncompressed(transcoder_format)) {
      unCompressed = true;
      const uint32_t bytes_per_pixel =
          basist::basis_get_uncompressed_bytes_per_pixel(transcoder_format);
      const uint32_t bytes_per_line = imageInfo.m_orig_width * bytes_per_pixel;
      const uint32_t bytes_per_slice = bytes_per_line * imageInfo.m_orig_height;
      dest_size = bytes_per_slice;
    } else {
      uint32_t bytes_per_block =
          basist::basis_get_bytes_per_block(transcoder_format);
      uint32_t required_size = imageInfo.m_total_blocks * bytes_per_block;

      if (transcoder_format ==
              basist::transcoder_texture_format::cTFPVRTC1_4_RGB ||
          transcoder_format ==
              basist::transcoder_texture_format::cTFPVRTC1_4_RGBA) {
        // For PVRTC1, Basis only writes (or requires) total_blocks *
        // bytes_per_block. But GL requires extra padding for very small
        // textures:
        // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
        // The transcoder will clear the extra bytes followed the used blocks to
        // 0.
        const uint32_t width = (imageInfo.m_orig_width + 3) & ~3;
        const uint32_t height = (imageInfo.m_orig_height + 3) & ~3;
        required_size =
            (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
      }
      dest_size = required_size;
    }

    dst_data.resize(dest_size);

    bool status = transcoder->transcode_image_level(
        buffer.data(), buffer.size(), 0, 0, dst_data.data(),
        imageInfo.m_orig_width * imageInfo.m_orig_height, transcoder_format, 0,
        imageInfo.m_orig_width, nullptr, imageInfo.m_orig_height);

    if (dst_data.data()) {
      glTexImage2D(GL_TEXTURE_2D, 0, format, imageInfo.m_orig_width,
                   imageInfo.m_orig_height, 0, format, internal_format,
                   dst_data.data());

      glGenerateMipmap(GL_TEXTURE_2D);
    }

    mWidth = imageInfo.m_width;
    mHeight = imageInfo.m_height;
  }

  explicit Texture(const std::string &imagePath, GLint format = GL_RGB,
                   bool flipImage = true)
      : mId(0) {

    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int height, width, channels;
    stbi_set_flip_vertically_on_load(flipImage);
    const char *path = imagePath.c_str();

    unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, (GLenum)format,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    mWidth = width;
    mHeight = height;
  }

  ~Texture() = default;

  unsigned int mId;
  unsigned int mWidth;
  unsigned int mHeight;

private:
  static bool hasExtension(std::string_view full, std::string_view end) {
    if (full.length() >= end.length()) {
      return (full.compare(full.length() - end.length(), full.length(), end) ==
              0);
    } else {
      return false;
    }
  }
};
} // namespace DL