#include "renderdevice.h"

#include "shader.h"
#include "texture.h"
#include <glad/glad.h>
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

class OpenGLRenderDevice final : public IRenderDevice {
public:
  auto createTexturedQuad() -> MeshHandle override {
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

  auto createBasisTexture(
      std::string_view path,
      basist::etc1_global_selector_codebook &codebook) -> TextureHandle override {
    auto texture = std::make_unique<Texture>(path, GL_TEXTURE0, codebook);
    if (texture->mId == 0) {
      return {};
    }
    return storeResource<TextureHandle>(std::move(texture), textures_);
  }

  auto createPipeline(std::string_view vertex_path,
                      std::string_view fragment_path,
                      std::string_view glsl_version) -> PipelineHandle override {
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

  void draw(const DrawCommand &command) override {
    auto mesh_it = meshes_.find(command.mesh.value);
    auto pipeline_it = pipelines_.find(command.pipeline.value);
    if (mesh_it == meshes_.end() || pipeline_it == pipelines_.end()) {
      return;
    }

    auto &mesh = *mesh_it->second;
    auto &pipeline = *pipeline_it->second;
    pipeline.use();

    if (command.texture.valid()) {
      auto texture_it = textures_.find(command.texture.value);
      if (texture_it != textures_.end()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_it->second->mId);
        pipeline.setInt("texture1", 0);
      }
    }

    for (const auto &uniform : command.uniforms) {
      if (uniform.type == UniformValue::Type::Float) {
        pipeline.setFloat(uniform.name, uniform.float_value);
      } else if (uniform.type == UniformValue::Type::Mat4) {
        auto matrix = uniform.mat4_value;
        pipeline.setMat4f(uniform.name, matrix);
      }
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }

private:
  template <typename Handle, typename T>
  auto storeResource(std::unique_ptr<T> resource,
                     std::unordered_map<std::size_t, std::unique_ptr<T>> &storage)
      -> Handle {
    const std::size_t id = next_resource_id_++;
    storage.emplace(id, std::move(resource));
    return Handle{id};
  }

  std::size_t next_resource_id_ = 1;
  std::unordered_map<std::size_t, std::unique_ptr<GLMesh>> meshes_;
  std::unordered_map<std::size_t, std::unique_ptr<Texture>> textures_;
  std::unordered_map<std::size_t, std::unique_ptr<Shader>> pipelines_;
};

} // namespace

auto createOpenGLRenderDevice() -> std::unique_ptr<IRenderDevice> {
  return std::make_unique<OpenGLRenderDevice>();
}

} // namespace DL
