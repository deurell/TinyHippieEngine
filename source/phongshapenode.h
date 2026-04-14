#pragma once

#include "camera.h"
#include "renderresourcecache.h"
#include "scenenode.h"
#include "shapevisualizer.h"
#include <memory>

enum class ShapeType {
  Cube,
  Sphere,
  Cylinder,
};

class PhongShapeNode : public DL::SceneNode {
public:
  PhongShapeNode(ShapeType shapeType, DL::IRenderDevice *renderDevice,
                 DL::RenderResourceCache *renderResourceCache = nullptr,
                 DL::SceneNode *parentNode = nullptr,
                 DL::Camera *camera = nullptr);
  ~PhongShapeNode() override = default;

  void init() override;
  void update(const DL::FrameContext &ctx) override;
  void render(const DL::FrameContext &ctx) override;
  void onScreenSizeChanged(glm::vec2 size) override;
  [[nodiscard]] std::string_view debugTypeName() const override {
    return "PhongShapeNode";
  }

  void setMaterial(const DL::PhongMaterial &material);

private:
  void initCamera();
  void initComponents();
  DL::GeneratedMeshData buildMeshData() const;

  ShapeType shapeType_;
  DL::IRenderDevice *renderDevice_ = nullptr;
  DL::RenderResourceCache *renderResourceCache_ = nullptr;
  std::unique_ptr<DL::Camera> localCamera_;
  DL::Camera *camera_ = nullptr;
  DL::ShapeVisualizer *visualizer_ = nullptr;
  DL::PhongMaterial material_;
};
