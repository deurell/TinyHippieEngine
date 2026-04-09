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
  void update(float) override {}
  void render(float) override {}
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

} // namespace
