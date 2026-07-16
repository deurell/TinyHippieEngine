#include "renderqueue.h"

#include "iscene.h"
#include <array>
#include <cmath>
#include <utility>

namespace DL {
namespace {

const glm::mat4 *findMat4Uniform(const RenderItem &item,
                                 std::string_view name) {
  for (const UniformValue &uniform : item.uniforms) {
    if (uniform.name == name && uniform.type == UniformValue::Type::Mat4) {
      return &uniform.mat4_value;
    }
  }
  return nullptr;
}

bool isClipSpaceOutsidePlane(const std::array<glm::vec4, 8> &corners,
                             int axis, float sign) {
  for (const glm::vec4 &corner : corners) {
    if (corner.w <= 0.0f) {
      return false;
    }
    if (sign * corner[axis] <= corner.w) {
      return false;
    }
  }
  return true;
}

bool isVisibleInClipSpace(const RenderItem &item) {
  const glm::mat4 *model = findMat4Uniform(item, "model");
  const glm::mat4 *view = findMat4Uniform(item, "view");
  const glm::mat4 *projection = findMat4Uniform(item, "projection");
  if (model == nullptr || view == nullptr || projection == nullptr) {
    return true;
  }

  const glm::mat4 mvp = (*projection) * (*view) * (*model);
  const glm::vec3 center = item.localBounds.center;
  const glm::vec3 half = item.localBounds.halfExtents;
  const std::array<glm::vec3, 8> localCorners = {
      center + glm::vec3{-half.x, -half.y, -half.z},
      center + glm::vec3{half.x, -half.y, -half.z},
      center + glm::vec3{-half.x, half.y, -half.z},
      center + glm::vec3{half.x, half.y, -half.z},
      center + glm::vec3{-half.x, -half.y, half.z},
      center + glm::vec3{half.x, -half.y, half.z},
      center + glm::vec3{-half.x, half.y, half.z},
      center + glm::vec3{half.x, half.y, half.z},
  };

  std::array<glm::vec4, 8> clipCorners{};
  for (std::size_t i = 0; i < localCorners.size(); ++i) {
    clipCorners[i] = mvp * glm::vec4(localCorners[i], 1.0f);
  }

  return !isClipSpaceOutsidePlane(clipCorners, 0, -1.0f) &&
         !isClipSpaceOutsidePlane(clipCorners, 0, 1.0f) &&
         !isClipSpaceOutsidePlane(clipCorners, 1, -1.0f) &&
         !isClipSpaceOutsidePlane(clipCorners, 1, 1.0f) &&
         !isClipSpaceOutsidePlane(clipCorners, 2, -1.0f) &&
         !isClipSpaceOutsidePlane(clipCorners, 2, 1.0f);
}

} // namespace

DrawCommand toDrawCommand(const RenderItem &item) {
  DrawCommand command;
  command.mesh = item.mesh;
  command.pipeline = item.pipeline;
  command.texture = item.texture;
  command.pass = item.pass;
  command.blendMode = item.blendMode;
  command.depthTest = item.depthTest;
  command.sortMode = item.sortMode;
  command.sortDepth = item.sortDepth;
  command.lineWidth = item.lineWidth;
  command.uniforms = item.uniforms;
  return command;
}

void RenderQueue::submit(RenderItem item) { items_.push_back(std::move(item)); }

void submitRenderItem(const FrameContext &ctx, IRenderDevice &renderDevice,
                      RenderItem item) {
  if (ctx.renderQueue != nullptr) {
    ctx.renderQueue->submit(std::move(item));
  } else {
    renderDevice.draw(toDrawCommand(item));
  }
}

RenderQueueStats RenderQueue::flush(IRenderDevice &renderDevice,
                                    RenderQueueOptions options) {
  const std::vector<RenderItem> items = std::move(items_);
  items_.clear();
  RenderQueueStats stats;
  stats.submittedItems = static_cast<std::uint32_t>(items.size());
  for (const RenderItem &item : items) {
    if (options.cullingEnabled && item.cullable &&
        !isVisibleInClipSpace(item)) {
      ++stats.culledItems;
      continue;
    }
    renderDevice.draw(toDrawCommand(item));
    ++stats.drawnItems;
  }
  return stats;
}

void RenderQueue::clear() { items_.clear(); }

} // namespace DL
