#pragma once

#include "camera.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "visualizerbase.h"
#include <vector>

namespace DL {
class PhysicsContext;
}

class PhysicsDebugVisualizer final : public DL::VisualizerBase {
public:
  PhysicsDebugVisualizer(DL::Camera &camera, DL::SceneNode &node,
                         DL::IRenderDevice &renderDevice,
                         const DL::PhysicsContext &physicsContext,
                         const std::vector<DL::PhysicsDebugLine> &extraLines);
  ~PhysicsDebugVisualizer() override;

  PhysicsDebugVisualizer(const PhysicsDebugVisualizer &) = delete;
  PhysicsDebugVisualizer &operator=(const PhysicsDebugVisualizer &) = delete;

  void render(const glm::mat4 &worldTransform, const DL::FrameContext &ctx,
              DL::RenderPassId pass) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhysicsDebugVisualizer";
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
