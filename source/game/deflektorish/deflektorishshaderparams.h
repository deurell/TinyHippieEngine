#pragma once

#include "shaderplanenode.h"
#include <glm/glm.hpp>

namespace Deflektorish {

inline void setSourceParams(DL::ShaderPlaneNode *node, float elapsed,
                            float pulse, float load) {
  if (node != nullptr) {
    node->config.params0 = {elapsed, pulse, load, 0.0f};
  }
}

inline void setReflectorParams(DL::ShaderPlaneNode *node, float glow,
                               bool automatic, bool selected, float energy,
                               bool hasHit = false,
                               glm::vec2 hitPoint = glm::vec2(0.0f)) {
  if (node != nullptr) {
    node->config.params0 = {glow, automatic ? 1.0f : 0.0f,
                            selected ? 1.0f : 0.0f, energy / 3.0f};
    node->config.params1.x = hitPoint.x;
    node->config.params1.y = hitPoint.y;
    node->config.params1.w = hasHit ? 1.0f : 0.0f;
  }
}

inline void setReflectorOcclusionMode(DL::ShaderPlaneNode *node,
                                      bool occludesBeam) {
  if (node != nullptr) {
    node->config.params1.z = occludesBeam ? 1.0f : 0.0f;
  }
}

inline void setBlockerParams(DL::ShaderPlaneNode *node, float glow,
                             float storedEnergy, glm::vec2 hitPoint) {
  if (node != nullptr) {
    node->config.params0 = {glow, storedEnergy, hitPoint.x, hitPoint.y};
  }
}

inline void setPortalParams(DL::ShaderPlaneNode *node, float elapsed,
                            float phase, float glow, bool exitPortal,
                            glm::vec2 hitPoint) {
  if (node != nullptr) {
    node->config.params0 = {elapsed, phase, glow, exitPortal ? 1.0f : 0.0f};
    node->config.params1 = {hitPoint.x, hitPoint.y, 0.0f, 0.0f};
  }
}

inline void setFilterParams(DL::ShaderPlaneNode *node, float passGlow,
                            float blockGlow, glm::vec2 hitPoint) {
  if (node != nullptr) {
    node->config.params0 = {passGlow, blockGlow, hitPoint.x, hitPoint.y};
  }
}

inline void setSplitterParams(DL::ShaderPlaneNode *node, float glow,
                              glm::vec2 hitPoint) {
  if (node != nullptr) {
    node->config.params0 = {glow, hitPoint.x, hitPoint.y, 0.0f};
  }
}

inline void setTargetParams(DL::ShaderPlaneNode *node, float elapsed,
                            float phase, float hitFlash) {
  if (node != nullptr) {
    node->config.params0 = {elapsed * 1.8f, phase, hitFlash, 0.0f};
  }
}

inline void setSelectionParams(DL::ShaderPlaneNode *node, float elapsed,
                               float flash) {
  if (node != nullptr) {
    node->config.params0 = {elapsed, flash, 0.0f, 0.0f};
  }
}

inline void setExplosionParams(DL::ShaderPlaneNode *node, float amount,
                               float energy, float seed, float visualScale) {
  if (node != nullptr) {
    node->config.params0 = {amount, energy / 3.0f, seed, visualScale};
  }
}

inline void setBeamSegmentParams(DL::ShaderPlaneNode *node, float energy,
                                 float lengthPixels, float textureRepeat) {
  if (node != nullptr) {
    node->config.params0 = {energy, lengthPixels, textureRepeat, 0.0f};
  }
}

inline void setBeamPulseParams(DL::ShaderPlaneNode *node, float elapsed,
                               float strength, float phase, float danger) {
  if (node != nullptr) {
    node->config.params1 = {elapsed, strength, phase, danger};
  }
}

inline void clearShaderParams(DL::ShaderPlaneNode *node) {
  if (node != nullptr) {
    node->config.params0 = {0.0f, 0.0f, 0.0f, 0.0f};
    node->config.params1 = {0.0f, 0.0f, 0.0f, 0.0f};
  }
}

inline void setEnergyBarParams(DL::ShaderPlaneNode *node, float ratio,
                               float danger, float drainRatio, float elapsed) {
  if (node != nullptr) {
    node->config.params0 = {ratio, danger, drainRatio, elapsed};
  }
}

inline void setFadeTransitionParams(DL::ShaderPlaneNode *node, float elapsed,
                                    float alpha, float progress,
                                    bool incoming) {
  if (node != nullptr) {
    node->config.params0 = {elapsed, alpha, progress, incoming ? 1.0f : 0.0f};
  }
}

inline void setCompletionOverlayParams(DL::ShaderPlaneNode *node, float time,
                                       float alpha, float blastProgress,
                                       float intro, float textFade,
                                       float backdrop, float bandA = 0.26f,
                                       float bandB = -0.10f) {
  if (node != nullptr) {
    node->config.params0 = {time, alpha, blastProgress, intro};
    node->config.params1 = {textFade, backdrop, bandA, bandB};
  }
}

} // namespace Deflektorish
