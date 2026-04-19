#include "iscene.h"
#include "scenemanager.h"
#include <gtest/gtest.h>
#include <string>

namespace {

class StubScene final : public DL::IScene {
public:
  explicit StubScene(std::string name) : name_(std::move(name)) {}

  void init() override {}
  void update(const DL::FrameContext &) override {}
  void render(const DL::FrameContext &) override {}
  void onClick(double, double) override {}
  void onKey(int) override {}
  void onScreenSizeChanged(glm::vec2) override {}

  std::string name_;
};

TEST(SceneManagerTest, ReturnsNullWithoutRegisteredScenes) {
  DL::SceneManager manager;

  EXPECT_FALSE(manager.hasScenes());
  EXPECT_EQ(manager.createCurrent(), nullptr);
}

TEST(SceneManagerTest, CreatesCurrentScene) {
  DL::SceneManager manager;
  manager.registerScene([] {
    return std::make_unique<StubScene>("current_scene");
  });

  auto scene = manager.createCurrent();
  auto *stubScene = dynamic_cast<StubScene *>(scene.get());

  ASSERT_NE(stubScene, nullptr);
  EXPECT_EQ(stubScene->name_, "current_scene");
}

TEST(SceneManagerTest, NextAndPreviousWrapAcrossRegisteredScenes) {
  DL::SceneManager manager;
  manager.registerScene([] {
    return std::make_unique<StubScene>("scene_0");
  });
  manager.registerScene([] {
    return std::make_unique<StubScene>("scene_1");
  });
  manager.registerScene([] {
    return std::make_unique<StubScene>("scene_2");
  });

  auto current = manager.createCurrent();
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_0");

  manager.next();
  current = manager.createCurrent();
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_1");

  manager.next();
  current = manager.createCurrent();
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_2");

  manager.next();
  current = manager.createCurrent();
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_0");

  manager.previous();
  current = manager.createCurrent();
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_2");
}

TEST(SceneManagerTest, NextAndPreviousAreNoOpsWithoutRegisteredScenes) {
  DL::SceneManager manager;

  manager.next();
  manager.previous();

  EXPECT_EQ(manager.createCurrent(), nullptr);
}

TEST(SceneManagerTest, PreviousWrapsToLastSceneFromStart) {
  DL::SceneManager manager;
  manager.registerScene([] {
    return std::make_unique<StubScene>("scene_0");
  });
  manager.registerScene([] {
    return std::make_unique<StubScene>("scene_1");
  });

  manager.previous();
  auto current = manager.createCurrent();

  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "scene_1");
}

TEST(SceneManagerTest, SingleSceneRemainsCurrentAcrossNavigation) {
  DL::SceneManager manager;
  manager.registerScene([] {
    return std::make_unique<StubScene>("only_scene");
  });

  manager.next();
  auto current = manager.createCurrent();
  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "only_scene");

  manager.previous();
  current = manager.createCurrent();
  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->name_, "only_scene");
}

} // namespace
