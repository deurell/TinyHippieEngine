#pragma once

#include "basisu_global_selector_palette.h"
#include "meshasset.h"
#include "renderdevice.h"
#include "visualizerbase.h"
#include <vector>

namespace DL {

struct MeshVisualizerSettings {
  glm::vec3 lightDirection = glm::normalize(glm::vec3(0.35f, 1.0f, 0.25f));
  glm::vec3 lightColor{1.0f, 0.96f, 0.9f};
  float ambientStrength = 0.42f;
  float specularStrength = 0.12f;
  float shininess = 24.0f;
};

class MeshVisualizer : public VisualizerBase {
public:
  MeshVisualizer(std::string name, DL::Camera &camera, SceneNode &node,
                 MeshAsset asset,
                 basist::etc1_global_selector_codebook *codeBook,
                 DL::IRenderDevice *renderDevice,
                 std::string vertexShaderPath = "Shaders/meshnode.vert",
                 std::string fragmentShaderPath = "Shaders/meshnode.frag");
  ~MeshVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const DL::FrameContext &ctx) override;
  void setDebugNormals(bool enabled) { debugNormals_ = enabled; }
  [[nodiscard]] bool debugNormals() const { return debugNormals_; }
  void setSettings(const MeshVisualizerSettings &settings) { settings_ = settings; }
  [[nodiscard]] const MeshVisualizerSettings &settings() const { return settings_; }

private:
  struct GpuSubmesh {
    MeshHandle mesh;
    TextureHandle texture;
    glm::vec3 diffuseColor{1.0f, 1.0f, 1.0f};
    glm::vec3 ambientColor{0.4f, 0.4f, 0.4f};
    glm::vec3 specularColor{0.2f, 0.2f, 0.2f};
    float shininess = 16.0f;
    bool hasTexture = false;
  };

  TextureHandle createFallbackTexture();
  TextureHandle loadTexture(const MeshAssetSubmesh &submesh);

  DL::IRenderDevice *renderDevice_ = nullptr;
  basist::etc1_global_selector_codebook *codeBook_ = nullptr;
  PipelineHandle pipeline_;
  MeshAsset asset_;
  std::vector<GpuSubmesh> submeshes_;
  bool debugNormals_ = false;
  MeshVisualizerSettings settings_;
};

} // namespace DL
