#include "scenenode.h"
#include <gtest/gtest.h>
#include <glm/gtx/quaternion.hpp>

namespace {

class TestSceneNode final : public DL::SceneNode {
public:
  using DL::SceneNode::SceneNode;
};

class TrackingSceneNode final : public DL::SceneNode {
public:
  using DL::SceneNode::SceneNode;

  void onScreenSizeChanged(glm::vec2 size) override {
    lastScreenSize = size;
    DL::SceneNode::onScreenSizeChanged(size);
  }

  glm::vec2 lastScreenSize{0.0f, 0.0f};
};

class TestVisualizer final : public DL::VisualizerBase {
public:
  TestVisualizer(std::string name, DL::Camera &camera, DL::SceneNode &node)
      : DL::VisualizerBase(camera, std::move(name), "", "", "", node) {}

  void render(const glm::mat4 &, float) override {}
};

TEST(SceneNodeTest, LocalTransformBecomesWorldTransformForRootNode) {
  TestSceneNode node;
  node.setLocalPosition({1.0f, 2.0f, 3.0f});
  node.setLocalScale({2.0f, 3.0f, 4.0f});

  node.update({});

  EXPECT_NEAR(node.getWorldPosition().x, 1.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldPosition().y, 2.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldPosition().z, 3.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().x, 2.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().y, 3.0f, 1e-6f);
  EXPECT_NEAR(node.getWorldScale().z, 4.0f, 1e-6f);
}

TEST(SceneNodeTest, ChildWorldPositionIncludesParentTransform) {
  auto child = std::make_unique<TestSceneNode>();
  auto *childPtr = child.get();

  TestSceneNode parent;
  parent.setLocalPosition({10.0f, 0.0f, 0.0f});
  childPtr->setLocalPosition({0.0f, 5.0f, 0.0f});
  parent.addChild(std::move(child));

  parent.update({});

  EXPECT_NEAR(childPtr->getWorldPosition().x, 10.0f, 1e-6f);
  EXPECT_NEAR(childPtr->getWorldPosition().y, 5.0f, 1e-6f);
  EXPECT_NEAR(childPtr->getWorldPosition().z, 0.0f, 1e-6f);
}

TEST(SceneNodeTest, MarkDirtyPropagatesToChildrenAfterParentChanges) {
  auto child = std::make_unique<TestSceneNode>();
  auto *childPtr = child.get();

  TestSceneNode parent;
  childPtr->setLocalPosition({1.0f, 0.0f, 0.0f});
  parent.addChild(std::move(child));

  parent.update({});
  EXPECT_NEAR(childPtr->getWorldPosition().x, 1.0f, 1e-6f);

  parent.setLocalPosition({2.0f, 0.0f, 0.0f});
  parent.update({});

  EXPECT_NEAR(childPtr->getWorldPosition().x, 3.0f, 1e-6f);
}

TEST(SceneNodeTest, WorldRotationTracksLocalRotation) {
  TestSceneNode node;
  const glm::quat rotation = glm::angleAxis(glm::half_pi<float>(),
                                            glm::vec3(0.0f, 0.0f, 1.0f));
  node.setLocalRotation(rotation);

  node.update({});

  const glm::quat worldRotation = node.getWorldRotation();
  EXPECT_NEAR(glm::angle(worldRotation), glm::angle(rotation), 1e-5f);
  EXPECT_NEAR(glm::axis(worldRotation).z, glm::axis(rotation).z, 1e-5f);
}

TEST(SceneNodeTest, GetVisualizerReturnsMatchingVisualizerByName) {
  TestSceneNode node;
  DL::Camera camera({0.0f, 0.0f, 1.0f});

  auto visualizer = std::make_unique<TestVisualizer>("main", camera, node);
  auto *visualizerPtr = visualizer.get();
  node.visualizers.emplace_back(std::move(visualizer));

  EXPECT_EQ(node.getVisualizer("main"), visualizerPtr);
  EXPECT_EQ(node.getVisualizer("missing"), nullptr);
}

TEST(SceneNodeTest, OnScreenSizeChangedPropagatesToChildren) {
  auto child = std::make_unique<TrackingSceneNode>();
  auto *childPtr = child.get();

  TrackingSceneNode parent;
  parent.addChild(std::move(child));

  parent.onScreenSizeChanged({1280.0f, 720.0f});

  EXPECT_EQ(parent.lastScreenSize, glm::vec2(1280.0f, 720.0f));
  EXPECT_EQ(childPtr->lastScreenSize, glm::vec2(1280.0f, 720.0f));
}

} // namespace
