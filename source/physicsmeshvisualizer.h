#pragma once

#include "camera.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "renderqueue.h"
#include "visualizerbase.h"
#include <vector>

class PhysicsMeshVisualizer final : public DL::VisualizerBase {
public:
  PhysicsMeshVisualizer(DL::Camera &camera, DL::SceneNode &node,
                        DL::IRenderDevice &renderDevice,
                        const DL::PhysicsShapeDesc &shape,
                        const glm::vec4 &color);
  ~PhysicsMeshVisualizer() override;

  PhysicsMeshVisualizer(const PhysicsMeshVisualizer &) = delete;
  PhysicsMeshVisualizer &operator=(const PhysicsMeshVisualizer &) = delete;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhysicsMeshVisualizer";
  }

private:
  void createMesh(const DL::PhysicsShapeDesc &shape, const glm::vec4 &color);

  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::MeshHandle mesh_;
  DL::TextureHandle texture_;
  DL::PipelineHandle pipeline_;
  DL::Bounds localBounds_;
  glm::vec3 baseTint_{1.0f};
  glm::vec3 ambientTint_{0.55f};
};
