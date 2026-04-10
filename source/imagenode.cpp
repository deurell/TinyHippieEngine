#include "imagenode.h"

#include <utility>

#include "imagevisualizer.h"
#include "planevisualizer.h"

ImageNode::ImageNode(std::string imagePath,
                     basist::etc1_global_selector_codebook *codeBook,
                     DL::IRenderDevice *renderDevice,
                     DL::SceneNode *parentNode)
    : SceneNode(parentNode), imagePath_(std::move(imagePath)), codeBook_(codeBook),
      renderDevice_(renderDevice) {}

void ImageNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void ImageNode::update(const DL::FrameContext &ctx) { SceneNode::update(ctx); }

void ImageNode::render(const DL::FrameContext &ctx) { SceneNode::render(ctx); }

void ImageNode::onScreenSizeChanged(glm::vec2 size) {
  SceneNode::onScreenSizeChanged(size);
  mScreenSize = size;
  mCamera->mScreenSize = size;
}

void ImageNode::initCamera() {
  mCamera = std::make_unique<DL::Camera>(glm::vec3(0, 0, 26));
  mCamera->lookAt({0, 0, 0});
}

void ImageNode::initComponents() {
  std::string vertexShaderPath = "Shaders/image.vert";
  std::string fragmentShaderPath = "Shaders/image.frag";
  
  auto visualizer = std::make_unique<DL::ImageVisualizer>(
      "ImageVisualizer", *mCamera, *this, imagePath_, codeBook_,
      renderDevice_);

  visualizers.emplace_back(std::move(visualizer));
}
