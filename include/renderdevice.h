#pragma once

#include "basisu_transcoder.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace DL {

struct MeshHandle {
  std::size_t value = 0;
  [[nodiscard]] bool valid() const { return value != 0; }
};

struct TextureHandle {
  std::size_t value = 0;
  [[nodiscard]] bool valid() const { return value != 0; }
};

struct PipelineHandle {
  std::size_t value = 0;
  [[nodiscard]] bool valid() const { return value != 0; }
};

struct UniformValue {
  enum class Type { Float, Mat4, Vec4 };

  static UniformValue makeFloat(std::string name, float value) {
    UniformValue uniform;
    uniform.name = std::move(name);
    uniform.type = Type::Float;
    uniform.float_value = value;
    return uniform;
  }

  static UniformValue makeMat4(std::string name, const glm::mat4 &value) {
    UniformValue uniform;
    uniform.name = std::move(name);
    uniform.type = Type::Mat4;
    uniform.mat4_value = value;
    return uniform;
  }

  static UniformValue makeVec4(std::string name, const glm::vec4 &value) {
    UniformValue uniform;
    uniform.name = std::move(name);
    uniform.type = Type::Vec4;
    uniform.vec4_value = value;
    return uniform;
  }

  std::string name;
  Type type = Type::Float;
  float float_value = 0.0f;
  glm::mat4 mat4_value = glm::mat4(1.0f);
  glm::vec4 vec4_value = glm::vec4(0.0f);
};

struct DrawCommand {
  MeshHandle mesh;
  PipelineHandle pipeline;
  TextureHandle texture;
  std::vector<UniformValue> uniforms;
};

class IRenderDevice {
public:
  virtual ~IRenderDevice() = default;

  virtual auto createTexturedQuad() -> MeshHandle = 0;
  virtual auto createMesh(const std::vector<glm::vec3> &positions,
                          const std::vector<glm::vec2> &uvs,
                          const std::vector<std::uint16_t> &indices)
      -> MeshHandle = 0;
  virtual auto createBasisTexture(
      std::string_view path,
      basist::etc1_global_selector_codebook &codebook) -> TextureHandle = 0;
  virtual auto createAlphaTexture(const std::uint8_t *pixels,
                                  std::uint32_t width,
                                  std::uint32_t height) -> TextureHandle = 0;
  virtual auto createPipeline(std::string_view vertex_path,
                              std::string_view fragment_path,
                              std::string_view glsl_version)
      -> PipelineHandle = 0;

  virtual void destroy(MeshHandle handle) = 0;
  virtual void destroy(TextureHandle handle) = 0;
  virtual void destroy(PipelineHandle handle) = 0;

  virtual void draw(const DrawCommand &command) = 0;
};

auto createOpenGLRenderDevice() -> std::unique_ptr<IRenderDevice>;

} // namespace DL
