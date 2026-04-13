#include "skinning.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace DL {
namespace {

glm::mat4 composeTransform(const glm::vec3 &translation, const glm::quat &rotation,
                           const glm::vec3 &scale) {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) *
         glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 localTransformForNode(const MeshAssetNode &node,
                                const AnimatedNodeTransform *animated) {
  const glm::vec3 translation =
      animated != nullptr && animated->hasTranslation ? animated->translation
                                                      : node.baseTranslation;
  const glm::quat rotation =
      animated != nullptr && animated->hasRotation ? animated->rotation
                                                   : node.baseRotation;
  const glm::vec3 scale =
      animated != nullptr && animated->hasScale ? animated->scale : node.baseScale;
  return composeTransform(translation, rotation, scale);
}

} // namespace

std::vector<glm::mat4> evaluateNodeWorldTransforms(const MeshAsset &asset,
                                                   const AnimationPose &pose) {
  std::vector<glm::mat4> worldTransforms(asset.nodes.size(), glm::mat4(1.0f));
  for (std::size_t nodeIndex = 0; nodeIndex < asset.nodes.size(); ++nodeIndex) {
    const auto &node = asset.nodes[nodeIndex];
    const AnimatedNodeTransform *animated =
        nodeIndex < pose.nodes.size() ? &pose.nodes[nodeIndex] : nullptr;
    const glm::mat4 localTransform = localTransformForNode(node, animated);
    if (node.parentIndex >= 0 &&
        static_cast<std::size_t>(node.parentIndex) < worldTransforms.size()) {
      worldTransforms[nodeIndex] =
          worldTransforms[static_cast<std::size_t>(node.parentIndex)] * localTransform;
    } else {
      worldTransforms[nodeIndex] = localTransform;
    }
  }
  return worldTransforms;
}

std::vector<glm::mat4> computeSkinMatrices(const MeshAsset &asset, int skinIndex,
                                           const AnimationPose &pose) {
  if (skinIndex < 0 || static_cast<std::size_t>(skinIndex) >= asset.skins.size()) {
    return {};
  }

  const auto worldTransforms = evaluateNodeWorldTransforms(asset, pose);
  const auto &skin = asset.skins[static_cast<std::size_t>(skinIndex)];
  std::vector<glm::mat4> skinMatrices(skin.jointNodeIndices.size(),
                                      glm::mat4(1.0f));

  for (std::size_t jointIndex = 0; jointIndex < skin.jointNodeIndices.size();
       ++jointIndex) {
    const int nodeIndex = skin.jointNodeIndices[jointIndex];
    if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= worldTransforms.size()) {
      continue;
    }

    const glm::mat4 inverseBind =
        jointIndex < skin.inverseBindMatrices.size()
            ? skin.inverseBindMatrices[jointIndex]
            : glm::mat4(1.0f);
    skinMatrices[jointIndex] =
        worldTransforms[static_cast<std::size_t>(nodeIndex)] * inverseBind;
  }

  return skinMatrices;
}

} // namespace DL
