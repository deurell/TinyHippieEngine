#include "imagenode.h"

#include <utility>

#include "imagevisualizer.h"
#include "planevisualizer.h"

ImageNode::ImageNode(std::string_view glslVersionString, std::string imagePath,
                     basist::etc1_global_selector_codebook *codeBook,
                     DL::SceneNode *parentNode)
    : SceneNode(parentNode), mGlslVersionString(glslVersionString.data()),
      imagePath_(std::move(imagePath)), codeBook_(codeBook) {}

void ImageNode::init() {
  SceneNode::init();
  initCamera();
  initComponents();
  setLocalPosition({0, 0, 0});
}

void ImageNode::update(float delta) { SceneNode::update(delta); }

void ImageNode::render(float delta) { SceneNode::render(delta); }

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
  
  auto texture = std::make_unique<DL::Texture>(imagePath_, GL_TEXTURE0, *codeBook_);
  auto visualizer = std::make_unique<DL::ImageVisualizer>(
      "ImageVisualizer", *mCamera, mGlslVersionString, *this,
      std::move(texture), codeBook_);

  visualizers.emplace_back(std::move(visualizer));
}
