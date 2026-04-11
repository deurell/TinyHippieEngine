#pragma once

#include "renderdevice.h"
#include "shapegeometry.h"
#include "visualizerbase.h"

namespace DL {

struct PhongMaterial {
  glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
  glm::vec3 ambient{0.25f, 0.25f, 0.25f};
  glm::vec3 specular{0.2f, 0.2f, 0.2f};
  float shininess = 16.0f;
};

class ShapeVisualizer : public VisualizerBase {
public:
  ShapeVisualizer(std::string name, Camera &camera, SceneNode &node,
                  GeneratedMeshData meshData, IRenderDevice *renderDevice,
                  std::string vertexShaderPath = "Shaders/phongshape.vert",
                  std::string fragmentShaderPath = "Shaders/phongshape.frag");
  ~ShapeVisualizer() override;

  void render(const glm::mat4 &worldTransform,
              const FrameContext &ctx) override;

  PhongMaterial material;
  glm::vec3 lightDirection = glm::normalize(glm::vec3(0.4f, 1.0f, 0.25f));
  glm::vec3 lightColor{1.0f, 0.95f, 0.9f};

private:
  IRenderDevice *renderDevice_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
};

} // namespace DL
