#pragma once

#include "camera.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "rendercomponent.h"
#include <vector>

namespace DL {
class PhysicsContext;
}

class PhysicsDebugRenderComponent final : public DL::RenderComponent {
public:
  PhysicsDebugRenderComponent(DL::Camera &camera, DL::SceneNode &node,
                         DL::IRenderDevice &renderDevice,
                         const DL::PhysicsContext &physicsContext,
                         const std::vector<DL::PhysicsDebugLine> &extraLines);
  ~PhysicsDebugRenderComponent() override;

  PhysicsDebugRenderComponent(const PhysicsDebugRenderComponent &) = delete;
  PhysicsDebugRenderComponent &operator=(const PhysicsDebugRenderComponent &) = delete;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhysicsDebugRenderComponent";
  }

private:
  void rebuildMesh(const std::vector<DL::PhysicsDebugLine> &lines);
  void destroyMesh();

  DL::IRenderDevice *renderDevice_ = nullptr;
  const DL::PhysicsContext *physicsContext_ = nullptr;
  const std::vector<DL::PhysicsDebugLine> *extraLines_ = nullptr;
  DL::MeshHandle mesh_;
  DL::PipelineHandle pipeline_;
};
