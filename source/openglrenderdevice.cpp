#include "renderdevice.h"

#include "shader.h"
#include "texture.h"
#include <glad/glad.h>
#include <string>
#include <unordered_map>

namespace DL {
namespace {

struct GLMesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  GLsizei index_count = 0;

  ~GLMesh() {
    if (vao != 0) {
      glDeleteVertexArrays(1, &vao);
    }
    if (vbo != 0) {
      glDeleteBuffers(1, &vbo);
    }
    if (ebo != 0) {
      glDeleteBuffers(1, &ebo);
    }
  }
};

struct GLTextureResource {
  GLuint id = 0;

  ~GLTextureResource() {
    if (id != 0) {
      glDeleteTextures(1, &id);
    }
  }
};

struct GLTextureFormat {
  GLint internalFormat = GL_RGBA8;
  GLenum uploadFormat = GL_RGBA;
};

GLTextureFormat toGLTextureFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::R8:
    return {GL_R8, GL_RED};
  case TextureFormat::RGB8:
    return {GL_RGB8, GL_RGB};
  case TextureFormat::RGBA8:
    return {GL_RGBA8, GL_RGBA};
  }
  return {GL_RGBA8, GL_RGBA};
}

class OpenGLRenderDevice final : public IRenderDevice {
public:
  explicit OpenGLRenderDevice(std::string glslVersion)
      : glslVersion_(std::move(glslVersion)) {}

  MeshHandle createTexturedQuad() override {
    static constexpr float vertices[] = {
        1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    static constexpr unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    auto mesh = std::make_unique<GLMesh>();
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    mesh->index_count = 6;
    return storeResource<MeshHandle>(std::move(mesh), meshes_);
  }

  MeshHandle createMesh(const std::vector<glm::vec3> &positions,
                        const std::vector<glm::vec2> &uvs,
                        const std::vector<std::uint16_t> &indices) override {
    if (positions.empty() || positions.size() != uvs.size() || indices.empty()) {
      return {};
    }

    std::vector<float> vertex_data;
    vertex_data.reserve(positions.size() * 5);
    for (std::size_t i = 0; i < positions.size(); ++i) {
      vertex_data.push_back(positions[i].x);
      vertex_data.push_back(positions[i].y);
      vertex_data.push_back(positions[i].z);
      vertex_data.push_back(uvs[i].x);
      vertex_data.push_back(uvs[i].y);
    }

    std::vector<unsigned int> index_data(indices.begin(), indices.end());

    auto mesh = std::make_unique<GLMesh>();
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_data.size() * sizeof(float)),
                 vertex_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(index_data.size() * sizeof(unsigned int)),
                 index_data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    mesh->index_count = static_cast<GLsizei>(index_data.size());
    return storeResource<MeshHandle>(std::move(mesh), meshes_);
  }

  TextureHandle createBasisTexture(
      std::string_view path,
      basist::etc1_global_selector_codebook &codebook) override {
    auto texture = std::make_unique<Texture>(path, GL_TEXTURE0, codebook);
    if (texture->mId == 0) {
      return {};
    }
    auto resource = std::make_unique<GLTextureResource>();
    resource->id = texture->mId;
    texture->mId = 0;
    return storeResource<TextureHandle>(std::move(resource), textures_);
  }

  TextureHandle createTexture(const TextureDesc &desc) override {
    if (desc.pixels == nullptr || desc.width == 0 || desc.height == 0) {
      return {};
    }

    auto texture = std::make_unique<GLTextureResource>();
    glGenTextures(1, &texture->id);
    if (texture->id == 0) {
      return {};
    }

    glBindTexture(GL_TEXTURE_2D, texture->id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    desc.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    const auto glFormat = toGLTextureFormat(desc.format);
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat.internalFormat,
                 static_cast<GLsizei>(desc.width),
                 static_cast<GLsizei>(desc.height), 0, glFormat.uploadFormat,
                 GL_UNSIGNED_BYTE, desc.pixels);
    if (desc.generateMipmaps) {
      glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    return storeResource<TextureHandle>(std::move(texture), textures_);
  }

  PipelineHandle createPipeline(std::string_view vertex_path,
                                std::string_view fragment_path) override {
    return createPipeline(vertex_path, fragment_path, glslVersion_);
  }

  PipelineHandle createPipeline(std::string_view vertex_path,
                                std::string_view fragment_path,
                                std::string_view glsl_version) override {
    auto shader =
        std::make_unique<Shader>(vertex_path, fragment_path, glsl_version);
    if (shader->mId == 0) {
      return {};
    }
    return storeResource<PipelineHandle>(std::move(shader), pipelines_);
  }

  void destroy(MeshHandle handle) override { meshes_.erase(handle.value); }
  void destroy(TextureHandle handle) override { textures_.erase(handle.value); }
  void destroy(PipelineHandle handle) override { pipelines_.erase(handle.value); }

  void setViewport(std::uint32_t width, std::uint32_t height) override {
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
  }

  void beginFrame(const FramePassDesc &desc) override {
    if (desc.depthMode == DepthMode::Less) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
    } else {
      glDisable(GL_DEPTH_TEST);
    }

    GLbitfield clearMask = 0;
    if (hasFlag(desc.clearFlags, ClearFlags::Color)) {
      glClearColor(desc.clearColor.r, desc.clearColor.g, desc.clearColor.b,
                   desc.clearColor.a);
      clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (hasFlag(desc.clearFlags, ClearFlags::Depth)) {
      clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearMask != 0) {
      glClear(clearMask);
    }
  }

  void endFrame() override {}

  void draw(const DrawCommand &command) override {
    auto mesh_it = meshes_.find(command.mesh.value);
    auto pipeline_it = pipelines_.find(command.pipeline.value);
    if (mesh_it == meshes_.end() || pipeline_it == pipelines_.end()) {
      return;
    }

    auto &mesh = *mesh_it->second;
    auto &pipeline = *pipeline_it->second;
    pipeline.use();

    if (command.blendMode == BlendMode::Opaque) {
      glDisable(GL_BLEND);
    } else {
      glEnable(GL_BLEND);
      if (command.blendMode == BlendMode::Alpha) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      } else if (command.blendMode == BlendMode::Additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      }
    }

    if (command.texture.valid()) {
      auto texture_it = textures_.find(command.texture.value);
      if (texture_it != textures_.end()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_it->second->id);
        pipeline.setInt("texture0", 0);
        pipeline.setInt("texture1", 0);
      }
    }

    for (const auto &uniform : command.uniforms) {
      if (uniform.type == UniformValue::Type::Int) {
        pipeline.setInt(uniform.name, uniform.int_value);
      } else if (uniform.type == UniformValue::Type::Float) {
        pipeline.setFloat(uniform.name, uniform.float_value);
      } else if (uniform.type == UniformValue::Type::Mat4) {
        auto matrix = uniform.mat4_value;
        pipeline.setMat4f(uniform.name, matrix);
      } else if (uniform.type == UniformValue::Type::Vec2) {
        pipeline.setVec2f(uniform.name, uniform.vec2_value);
      } else if (uniform.type == UniformValue::Type::Vec3) {
        pipeline.setVec3f(uniform.name, uniform.vec3_value);
      } else if (uniform.type == UniformValue::Type::Vec4) {
        pipeline.setVec4f(uniform.name, uniform.vec4_value);
      }
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    if (command.blendMode != BlendMode::Opaque) {
      glDisable(GL_BLEND);
    }
  }

private:
  template <typename Handle, typename T>
  Handle storeResource(
      std::unique_ptr<T> resource,
      std::unordered_map<std::size_t, std::unique_ptr<T>> &storage) {
    const std::size_t id = next_resource_id_++;
    storage.emplace(id, std::move(resource));
    return Handle{id};
  }

  std::size_t next_resource_id_ = 1;
  std::string glslVersion_;
  std::unordered_map<std::size_t, std::unique_ptr<GLMesh>> meshes_;
  std::unordered_map<std::size_t, std::unique_ptr<GLTextureResource>> textures_;
  std::unordered_map<std::size_t, std::unique_ptr<Shader>> pipelines_;
};

} // namespace

std::unique_ptr<IRenderDevice> createOpenGLRenderDevice(
    std::string_view glslVersion) {
  return std::make_unique<OpenGLRenderDevice>(std::string(glslVersion));
}

std::unique_ptr<IRenderDevice> createOpenGLRenderDevice() {
#ifdef __EMSCRIPTEN__
  return createOpenGLRenderDevice("#version 300 es\n");
#else
  return createOpenGLRenderDevice("#version 330 core\n");
#endif
}

} // namespace DL
