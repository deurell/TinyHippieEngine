#pragma once

#include "stb_image.h"
#include <glad/glad.h>
#include <string>

class Texture {
public:
  Texture(const std::string &imagePath, GLint format = GL_RGB,
          bool flipImage = true) {

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
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
  }

  ~Texture() = default;

  unsigned int mId;
};
