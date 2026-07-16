#include "renderqueue.h"
#include <gtest/gtest.h>

namespace {

class RecordingRenderDevice final : public DL::IRenderDevice {
public:
  DL::MeshHandle createTexturedQuad() override { return {.value = 1}; }
  DL::MeshHandle createMesh(
      const std::vector<glm::vec3> &, const std::vector<glm::vec3> &,
      const std::vector<glm::vec2> &, const std::vector<std::uint32_t> &,
      const std::vector<std::array<std::uint16_t, 4>> & = {},
      const std::vector<glm::vec4> & = {}) override {
    return {.value = 1};
  }
  DL::MeshHandle createColoredMesh(const std::vector<glm::vec3> &,
                                   const std::vector<glm::vec4> &,
                                   const std::vector<std::uint32_t> &,
                                   DL::PrimitiveType = DL::PrimitiveType::Triangles) override {
    return {.value = 1};
  }
  DL::TextureHandle createBasisTexture(
      std::string_view, basist::etc1_global_selector_codebook &) override {
    return {.value = 1};
  }
  DL::TextureHandle createTexture(const DL::TextureDesc &) override {
    return {.value = 1};
  }
  DL::RenderTargetHandle createRenderTarget(std::uint32_t,
                                            std::uint32_t) override {
    return {.value = 1};
  }
  DL::PipelineHandle createPipeline(std::string_view,
                                    std::string_view) override {
    return {.value = 1};
  }
  DL::PipelineHandle createPipeline(std::string_view, std::string_view,
                                    std::string_view) override {
    return {.value = 1};
  }

  void destroy(DL::MeshHandle) override {}
  void destroy(DL::TextureHandle) override {}
  void destroy(DL::PipelineHandle) override {}
  void destroy(DL::RenderTargetHandle) override {}
  void setViewport(std::uint32_t, std::uint32_t) override {}
  void resizeRenderTarget(DL::RenderTargetHandle, std::uint32_t,
                          std::uint32_t) override {}
  [[nodiscard]] DL::TextureHandle
  getRenderTargetColorTexture(DL::RenderTargetHandle) const override {
    return {.value = 1};
  }
  void beginFrame(const DL::FramePassDesc &) override {}
  void endFrame() override {}
  void draw(const DL::DrawCommand &command) override {
    commands.push_back(command);
  }
  [[nodiscard]] DL::RenderStats getRenderStats() const override { return {}; }

  std::vector<DL::DrawCommand> commands;
};

DL::RenderItem makeItem(std::size_t id) {
  DL::RenderItem item;
  item.cullable = false;
  item.mesh = {.value = id};
  item.pipeline = {.value = 1};
  return item;
}

} // namespace

TEST(RenderQueueTest, DrawsOpaqueBeforeTransparentBackToFrontItems) {
  DL::RenderQueue queue;
  RecordingRenderDevice renderDevice;

  auto nearTransparent = makeItem(1);
  nearTransparent.blendMode = DL::BlendMode::Alpha;
  nearTransparent.sortMode = DL::DrawSortMode::BackToFront;
  nearTransparent.sortDepth = 2.0f;
  queue.submit(nearTransparent);

  auto opaque = makeItem(2);
  queue.submit(opaque);

  auto farTransparent = makeItem(3);
  farTransparent.blendMode = DL::BlendMode::Alpha;
  farTransparent.sortMode = DL::DrawSortMode::BackToFront;
  farTransparent.sortDepth = 8.0f;
  queue.submit(farTransparent);

  const DL::RenderQueueStats stats = queue.flush(renderDevice);

  ASSERT_EQ(renderDevice.commands.size(), 3u);
  EXPECT_EQ(renderDevice.commands[0].mesh.value, 2u);
  EXPECT_EQ(renderDevice.commands[1].mesh.value, 3u);
  EXPECT_EQ(renderDevice.commands[2].mesh.value, 1u);
  EXPECT_EQ(renderDevice.commands[1].sortMode, DL::DrawSortMode::None);
  EXPECT_EQ(stats.submittedItems, 3u);
  EXPECT_EQ(stats.drawnItems, 3u);
  EXPECT_EQ(stats.culledItems, 0u);
}

TEST(RenderQueueTest, PreservesTransparentSubmissionOrderForEqualDepth) {
  DL::RenderQueue queue;
  RecordingRenderDevice renderDevice;

  auto first = makeItem(10);
  first.blendMode = DL::BlendMode::Alpha;
  first.sortMode = DL::DrawSortMode::BackToFront;
  first.sortDepth = 4.0f;
  queue.submit(first);

  auto second = makeItem(11);
  second.blendMode = DL::BlendMode::Alpha;
  second.sortMode = DL::DrawSortMode::BackToFront;
  second.sortDepth = 4.0f;
  queue.submit(second);

  queue.flush(renderDevice);

  ASSERT_EQ(renderDevice.commands.size(), 2u);
  EXPECT_EQ(renderDevice.commands[0].mesh.value, 10u);
  EXPECT_EQ(renderDevice.commands[1].mesh.value, 11u);
}

TEST(RenderQueueTest, RenderLayerOverridesOpaqueAndTransparentBuckets) {
  DL::RenderQueue queue;
  RecordingRenderDevice renderDevice;

  auto higherLayerOpaque = makeItem(20);
  higherLayerOpaque.renderLayer = 10;
  queue.submit(higherLayerOpaque);

  auto lowerLayerTransparent = makeItem(21);
  lowerLayerTransparent.renderLayer = -1;
  lowerLayerTransparent.blendMode = DL::BlendMode::Alpha;
  lowerLayerTransparent.sortMode = DL::DrawSortMode::BackToFront;
  lowerLayerTransparent.sortDepth = 1.0f;
  queue.submit(lowerLayerTransparent);

  auto defaultLayerOpaque = makeItem(22);
  queue.submit(defaultLayerOpaque);

  queue.flush(renderDevice);

  ASSERT_EQ(renderDevice.commands.size(), 3u);
  EXPECT_EQ(renderDevice.commands[0].mesh.value, 21u);
  EXPECT_EQ(renderDevice.commands[1].mesh.value, 22u);
  EXPECT_EQ(renderDevice.commands[2].mesh.value, 20u);
}
