#pragma once

#include "camera.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "renderqueue.h"
#include "rendercomponent.h"
#include <vector>

class PhysicsMeshRenderComponent final : public DL::RenderComponent {
public:
  PhysicsMeshRenderComponent(DL::Camera &camera, DL::SceneNode &node,
                        DL::IRenderDevice &renderDevice,
                        const DL::PhysicsShapeDesc &shape,
                        const glm::vec4 &color);
  ~PhysicsMeshRenderComponent() override;

  PhysicsMeshRenderComponent(const PhysicsMeshRenderComponent &) = delete;
  PhysicsMeshRenderComponent &operator=(const PhysicsMeshRenderComponent &) = delete;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhysicsMeshRenderComponent";
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
