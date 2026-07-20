#pragma once

#include "game/deflektorish/beamworld.h"
#include "renderdevice.h"
#include "renderresourcecache.h"
#include <glm/glm.hpp>
#include <vector>

namespace DL {
class Camera;
class SceneNode;
class ShaderPlaneNode;
} // namespace DL

namespace Deflektorish {

class Renderer {
public:
  void createBeamSegments(DL::SceneNode *parent, DL::Camera *camera,
                          DL::IRenderDevice *renderDevice,
                          DL::RenderResourceCache *renderResourceCache,
                          int maxSegments);
  void updateBeamSegments(const BeamSolveResult &result);
  void updateBeamPulse(float elapsed, float strength);
  void updateSource(DL::ShaderPlaneNode *node, float elapsed, float pulse,
                    float load);
  void updateReflektor(DL::ShaderPlaneNode *node, float &glow, bool active,
                       bool automatic, bool selected, float energy, float dt);
  void updateBlocker(DL::ShaderPlaneNode *node, float &glow,
                     float &storedEnergy, glm::vec2 &hitPoint, bool active,
                     float energy, bool hasHit, glm::vec2 hit, float dt);
  void updatePortal(DL::ShaderPlaneNode *entryNode,
                    DL::ShaderPlaneNode *exitNode, float &glow,
                    glm::vec2 &entryHitPoint, glm::vec2 &exitHitPoint,
                    float phase, bool active, glm::vec2 entryHit,
                    glm::vec2 exitHit, bool hasHit, float elapsed, float dt);
  void updateFilter(DL::ShaderPlaneNode *node, float &passGlow,
                    float &blockGlow, glm::vec2 &hitPoint, bool passing,
                    bool blocked, bool hasHit, glm::vec2 hit, float dt);
  void updateSplitter(DL::ShaderPlaneNode *node, float &glow,
                      glm::vec2 &hitPoint, bool active, bool hasHit,
                      glm::vec2 hit, float dt);
  void updateTarget(DL::ShaderPlaneNode *node, bool alive, float hitFlash,
                    float elapsed, float phase);
  void updateSelection(DL::ShaderPlaneNode *node, glm::vec2 worldPosition,
                       float elapsed, float flash);
  void hideNode(DL::ShaderPlaneNode *node);
  void showExplosion(DL::ShaderPlaneNode *node, glm::vec2 position,
                     float energy);
  void updateExplosion(DL::ShaderPlaneNode *node, float amount, float energy,
                       float seed, bool active);
  [[nodiscard]] std::size_t beamSegmentCapacity() const;

private:
  struct BeamSegmentNode {
    DL::ShaderPlaneNode *node = nullptr;
    float energy = 0.0f;
    bool visible = false;
  };

  void layoutSegment(BeamSegmentNode &segment, glm::vec2 start, glm::vec2 end,
                     float energy);
  void hideSegment(BeamSegmentNode &segment);
  static float approach(float current, float target, float blend);
  static glm::vec2 toWorld(glm::vec2 pixels);

  std::vector<BeamSegmentNode> beamSegments_;
};

} // namespace Deflektorish
