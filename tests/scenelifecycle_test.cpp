#include "scenelifecycle.h"
#include <gtest/gtest.h>
#include <vector>

namespace {

class TrackingScene final : public DL::IScene {
public:
  void init() override { calls.emplace_back("init"); }
  void update(const DL::FrameContext &) override {}
  void render(const DL::FrameContext &) override {}
  void onClick(double, double) override {}
  void onKey(int) override {}
  void onScreenSizeChanged(glm::vec2 size) override {
    calls.emplace_back("screen");
    screenSize = size;
  }
  void onFramebufferSizeChanged(glm::vec2 size) override {
    calls.emplace_back("framebuffer");
    framebufferSize = size;
  }

  std::vector<const char *> calls;
  glm::vec2 screenSize{0.0f, 0.0f};
  glm::vec2 framebufferSize{0.0f, 0.0f};
};

TEST(SceneLifecycleTest, PrepareSceneReturnsNullForNullScene) {
  auto scene = DL::prepareScene(nullptr, {1024.0f, 768.0f}, {1024.0f, 768.0f});

  EXPECT_EQ(scene, nullptr);
}

TEST(SceneLifecycleTest, PrepareSceneInitializesSceneAndDeliversInitialSizes) {
  auto scene = std::make_unique<TrackingScene>();
  auto *scenePtr = scene.get();

  auto prepared = DL::prepareScene(std::move(scene), {1280.0f, 720.0f},
                                   {2560.0f, 1440.0f});

  ASSERT_NE(prepared, nullptr);
  EXPECT_EQ(prepared.get(), scenePtr);
  EXPECT_EQ(scenePtr->screenSize, glm::vec2(1280.0f, 720.0f));
  EXPECT_EQ(scenePtr->framebufferSize, glm::vec2(2560.0f, 1440.0f));
}

TEST(SceneLifecycleTest, PrepareSceneCallsLifecycleStepsInOrder) {
  auto scene = std::make_unique<TrackingScene>();
  auto *scenePtr = scene.get();

  auto prepared = DL::prepareScene(std::move(scene), {800.0f, 600.0f},
                                   {1600.0f, 1200.0f});

  ASSERT_NE(prepared, nullptr);
  ASSERT_EQ(scenePtr->calls.size(), 3u);
  EXPECT_STREQ(scenePtr->calls[0], "init");
  EXPECT_STREQ(scenePtr->calls[1], "screen");
  EXPECT_STREQ(scenePtr->calls[2], "framebuffer");
}

TEST(SceneLifecycleTest, ReplacePreparedSceneReturnsCurrentSceneWhenNextIsNull) {
  auto currentScene = std::make_unique<TrackingScene>();
  auto *currentScenePtr = currentScene.get();

  auto replaced = DL::replacePreparedScene(std::move(currentScene), nullptr,
                                           {1024.0f, 768.0f},
                                           {1024.0f, 768.0f});

  ASSERT_NE(replaced, nullptr);
  EXPECT_EQ(replaced.get(), currentScenePtr);
  EXPECT_TRUE(currentScenePtr->calls.empty());
}

TEST(SceneLifecycleTest, ReplacePreparedSceneReplacesCurrentSceneWhenNextIsValid) {
  auto currentScene = std::make_unique<TrackingScene>();
  auto *currentScenePtr = currentScene.get();
  auto nextScene = std::make_unique<TrackingScene>();
  auto *nextScenePtr = nextScene.get();

  auto replaced = DL::replacePreparedScene(std::move(currentScene),
                                           std::move(nextScene),
                                           {640.0f, 480.0f},
                                           {1280.0f, 960.0f});

  ASSERT_NE(replaced, nullptr);
  EXPECT_EQ(replaced.get(), nextScenePtr);
  EXPECT_NE(replaced.get(), currentScenePtr);
  EXPECT_EQ(nextScenePtr->screenSize, glm::vec2(640.0f, 480.0f));
  EXPECT_EQ(nextScenePtr->framebufferSize, glm::vec2(1280.0f, 960.0f));
}

} // namespace
