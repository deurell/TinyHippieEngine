#include "phongshapenode.h"

PhongShapeNode::PhongShapeNode(ShapeType shapeType,
                               DL::IRenderDevice *renderDevice,
                               DL::SceneNode *parentNode, DL::Camera *camera)
    : DL::SceneNode(parentNode), shapeType_(shapeType),
      renderDevice_(renderDevice), camera_(camera) {}

void PhongShapeNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
}

void PhongShapeNode::update(const DL::FrameContext &ctx) {
  SceneNode::update(ctx);
}

void PhongShapeNode::render(const DL::FrameContext &ctx) {
  SceneNode::render(ctx);
}

void PhongShapeNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  if (camera_ != nullptr) {
    camera_->mScreenSize = size;
  }
}

void PhongShapeNode::setMaterial(const DL::PhongMaterial &material) {
  material_ = material;
  if (visualizer_ != nullptr) {
    visualizer_->material = material_;
  }
}

void PhongShapeNode::initCamera() {
  if (camera_ != nullptr) {
    return;
  }
  localCamera_ = std::make_unique<DL::Camera>(glm::vec3(0.0f, 0.0f, 12.0f));
  localCamera_->lookAt({0.0f, 0.0f, 0.0f});
  camera_ = localCamera_.get();
}

void PhongShapeNode::initComponents() {
  if (camera_ == nullptr || renderDevice_ == nullptr) {
    return;
  }
  auto visualizer = std::make_unique<DL::ShapeVisualizer>(
      "ShapeVisualizer", *camera_, *this, buildMeshData(), renderDevice_);
  visualizer_ = visualizer.get();
  visualizer_->material = material_;
  visualizers.emplace_back(std::move(visualizer));
}

DL::GeneratedMeshData PhongShapeNode::buildMeshData() const {
  switch (shapeType_) {
  case ShapeType::Cube:
    return DL::makeCubeMesh();
  case ShapeType::Sphere:
    return DL::makeSphereMesh();
  case ShapeType::Cylinder:
    return DL::makeCylinderMesh();
  }
  return DL::makeCubeMesh();
}
