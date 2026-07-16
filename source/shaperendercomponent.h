#pragma once

#include "renderdevice.h"
#include "renderqueue.h"
#include "renderresourcecache.h"
#include "shapegeometry.h"
#include "rendercomponent.h"

namespace DL {

struct PhongMaterial {
  glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
  glm::vec3 ambient{0.25f, 0.25f, 0.25f};
  glm::vec3 specular{0.2f, 0.2f, 0.2f};
  float shininess = 16.0f;
};

class ShapeRenderComponent : public RenderComponent {
public:
  ShapeRenderComponent(Camera &camera, SceneNode &node, GeneratedMeshData meshData,
                  IRenderDevice *renderDevice,
                  RenderResourceCache *resourceCache = nullptr,
                  std::string vertexShaderPath = "Shaders/phongshape.vert",
                  std::string fragmentShaderPath = "Shaders/phongshape.frag");
  ~ShapeRenderComponent() override;

  void render(const glm::mat4 &worldTransform,
              const FrameContext &ctx, RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "ShapeRenderComponent";
  }

  PhongMaterial material;
  glm::vec3 lightDirection = glm::normalize(glm::vec3(0.4f, 1.0f, 0.25f));
  glm::vec3 lightColor{1.0f, 0.95f, 0.9f};

private:
  IRenderDevice *renderDevice_ = nullptr;
  RenderResourceCache *resourceCache_ = nullptr;
  MeshHandle mesh_;
  PipelineHandle pipeline_;
  Bounds localBounds_;
};

} // namespace DL
