#include "iscene.h"
#include "scenemanager.h"
#include <gtest/gtest.h>
#include <string>

namespace {

class StubScene final : public DL::IScene {
public:
  explicit StubScene(std::string glslVersion)
      : glslVersion_(std::move(glslVersion)) {}

  void init() override {}
  void update(const DL::FrameContext &) override {}
  void render(const DL::FrameContext &) override {}
  void onClick(double, double) override {}
  void onKey(int) override {}
  void onScreenSizeChanged(glm::vec2) override {}

  std::string glslVersion_;
};

TEST(SceneManagerTest, ReturnsNullWithoutRegisteredScenes) {
  DL::SceneManager manager;

  EXPECT_FALSE(manager.hasScenes());
  EXPECT_EQ(manager.createCurrent("#version 330 core\n"), nullptr);
}

TEST(SceneManagerTest, CreatesCurrentSceneWithProvidedGlslVersion) {
  DL::SceneManager manager;
  manager.registerScene([](std::string_view glslVersion) {
    return std::make_unique<StubScene>(std::string(glslVersion));
  });

  auto scene = manager.createCurrent("#version 330 core\n");
  auto *stubScene = dynamic_cast<StubScene *>(scene.get());

  ASSERT_NE(stubScene, nullptr);
  EXPECT_EQ(stubScene->glslVersion_, "#version 330 core\n");
}

TEST(SceneManagerTest, NextAndPreviousWrapAcrossRegisteredScenes) {
  DL::SceneManager manager;
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("scene_0");
  });
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("scene_1");
  });
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("scene_2");
  });

  auto current = manager.createCurrent("");
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_0");

  manager.next();
  current = manager.createCurrent("");
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_1");

  manager.next();
  current = manager.createCurrent("");
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_2");

  manager.next();
  current = manager.createCurrent("");
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_0");

  manager.previous();
  current = manager.createCurrent("");
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_2");
}

TEST(SceneManagerTest, NextAndPreviousAreNoOpsWithoutRegisteredScenes) {
  DL::SceneManager manager;

  manager.next();
  manager.previous();

  EXPECT_EQ(manager.createCurrent(""), nullptr);
}

TEST(SceneManagerTest, PreviousWrapsToLastSceneFromStart) {
  DL::SceneManager manager;
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("scene_0");
  });
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("scene_1");
  });

  manager.previous();
  auto current = manager.createCurrent("");

  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "scene_1");
}

TEST(SceneManagerTest, SingleSceneRemainsCurrentAcrossNavigation) {
  DL::SceneManager manager;
  manager.registerScene([](std::string_view) {
    return std::make_unique<StubScene>("only_scene");
  });

  manager.next();
  auto current = manager.createCurrent("");
  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "only_scene");

  manager.previous();
  current = manager.createCurrent("");
  ASSERT_NE(current, nullptr);
  EXPECT_EQ(dynamic_cast<StubScene *>(current.get())->glslVersion_, "only_scene");
}

} // namespace
