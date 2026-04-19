#include "iscene.h"
#include <gtest/gtest.h>

namespace {

class StubScene final : public DL::IScene {
public:
  void init() override {}
  void update(const DL::FrameContext &) override {}
  void render(const DL::FrameContext &) override {}
  void onClick(double, double) override {}
  void onKey(int) override {}
  void onScreenSizeChanged(glm::vec2 size) override { lastScreenSize = size; }

  glm::vec2 lastScreenSize{0.0f, 0.0f};
};

TEST(ISceneTest, FramebufferSizeDefaultsToScreenSizeCallback) {
  StubScene scene;

  scene.onFramebufferSizeChanged({1920.0f, 1080.0f});

  EXPECT_EQ(scene.lastScreenSize, glm::vec2(1920.0f, 1080.0f));
}

} // namespace
