#pragma once

#include "animationclip.h"
#include "meshasset.h"
#include <glm/glm.hpp>
#include <vector>

namespace DL {

std::vector<glm::mat4> evaluateNodeWorldTransforms(const MeshAsset &asset,
                                                   const AnimationPose &pose);
std::vector<glm::mat4> computeSkinMatrices(const MeshAsset &asset, int skinIndex,
                                           const AnimationPose &pose);

} // namespace DL
