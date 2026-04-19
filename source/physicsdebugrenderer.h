#pragma once

#include "camera.h"
#include "physicsworld.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include <vector>

namespace DL {

class PhysicsDebugRenderer {
public:
  PhysicsDebugRenderer(Camera &camera, IRenderDevice &renderDevice,
                       RenderResourceCache *resourceCache = nullptr);
  ~PhysicsDebugRenderer();

  PhysicsDebugRenderer(const PhysicsDebugRenderer &) = delete;
  PhysicsDebugRenderer &operator=(const PhysicsDebugRenderer &) = delete;

  void render(const std::vector<PhysicsDebugLine> &lines);

private:
  Camera &camera_;
  IRenderDevice &renderDevice_;
  RenderResourceCache *resourceCache_ = nullptr;
  PipelineHandle pipeline_;
  MeshHandle mesh_;
};

} // namespace DL
