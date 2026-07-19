#include "beamworld.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Deflektorish {
namespace {

constexpr float kMirrorLength = 44.0f;
constexpr float kTargetRadius = 14.0f;
constexpr float kBlockerHalfSize = 16.0f;
constexpr float kFilterHalfSize = 17.0f;
constexpr float kFilterPassHalfAngle = 14.0f * 3.1415926535f / 180.0f;
constexpr float kSplitterRadius = 18.0f;
constexpr float kSplitterBranchAngle = 35.0f * 3.1415926535f / 180.0f;
constexpr float kBeamRange = 2000.0f;
constexpr float kEpsilon = 0.001f;
constexpr float kHitGap = 6.0f;

float cross(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }

glm::vec2 direction(float angle) {
  return {std::cos(angle), std::sin(angle)};
}

glm::vec2 rotateVec(glm::vec2 value, float angle) {
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  return {value.x * c - value.y * s, value.x * s + value.y * c};
}

glm::vec2 reflectAcrossMirror(glm::vec2 ray, glm::vec2 mirrorDir) {
  const glm::vec2 normal = glm::normalize(glm::vec2{-mirrorDir.y, mirrorDir.x});
  return ray - normal * (2.0f * glm::dot(ray, normal));
}

glm::vec2 reflectFromNormal(glm::vec2 ray, glm::vec2 normal) {
  return ray - normal * (2.0f * glm::dot(ray, normal));
}

struct Hit {
  enum class Type { None, Reflektor, Target, Blocker, Portal, Filter, Splitter };
  Type type = Type::None;
  float distance = std::numeric_limits<float>::max();
  float exitDistance = std::numeric_limits<float>::max();
  glm::vec2 point{0.0f};
  int index = -1;
  glm::vec2 mirrorDir{0.0f};
  glm::vec2 normal{0.0f};
  bool passesFilter = false;
};

bool closer(const Hit &hit, const Hit &nearest) {
  return hit.type != Hit::Type::None && hit.distance < nearest.distance;
}

struct BeamRay {
  glm::vec2 origin{0.0f};
  glm::vec2 visualOrigin{0.0f};
  glm::vec2 rayDir{1.0f, 0.0f};
  int ignoreReflektor = -1;
  int ignoreBlocker = -1;
  int ignorePortal = -1;
  int ignoreFilter = -1;
  int ignoreSplitter = -1;
  float energy = 0.0f;
  int depth = 0;
};

void addSegment(BeamSolveResult &result, glm::vec2 start, glm::vec2 end,
                float energy, std::size_t maxSegments,
                BeamSegmentEnd segmentEnd = BeamSegmentEnd::None) {
  if (result.segments.size() >= maxSegments) {
    return;
  }
  result.segments.push_back({start, end, energy, segmentEnd});
}

} // namespace

BeamSolveResult solveBeamWorld(const BeamWorld &world,
                               std::size_t maxSegments) {
  BeamSolveResult result;
  result.activeReflektors.assign(world.reflektors.size(), false);
  result.reflektorEnergy.assign(world.reflektors.size(), 0.0f);
  result.activeBlockers.assign(world.blockers.size(), false);
  result.blockerEnergy.assign(world.blockers.size(), 0.0f);
  result.blockerHit.assign(world.blockers.size(), glm::vec2(0.0f));
  result.blockerHasHit.assign(world.blockers.size(), false);
  result.hitTargets.assign(world.targets.size(), false);
  result.targetEnergy.assign(world.targets.size(), 0.0f);
  result.activePortals.assign(world.portals.size(), false);
  result.portalEntryHit.assign(world.portals.size(), glm::vec2(0.0f));
  result.portalExitHit.assign(world.portals.size(), glm::vec2(0.0f));
  result.portalHasHit.assign(world.portals.size(), false);
  result.passingFilters.assign(world.filters.size(), false);
  result.blockedFilters.assign(world.filters.size(), false);
  result.filterHit.assign(world.filters.size(), glm::vec2(0.0f));
  result.filterHasHit.assign(world.filters.size(), false);
  result.activeSplitters.assign(world.splitters.size(), false);
  result.splitterHit.assign(world.splitters.size(), glm::vec2(0.0f));
  result.splitterHasHit.assign(world.splitters.size(), false);

  std::vector<BeamRay> rays;
  rays.push_back(
      {world.sourcePosition, world.sourcePosition, direction(world.sourceAngle)});

  while (!rays.empty() && result.segments.size() < maxSegments) {
    BeamRay ray = rays.back();
    rays.pop_back();

    while (result.segments.size() < maxSegments) {
      Hit nearest;
      for (std::size_t i = 0; i < world.reflektors.size(); ++i) {
        if (static_cast<int>(i) == ray.ignoreReflektor) {
          continue;
        }
        const Reflektor &reflektor = world.reflektors[i];
        const glm::vec2 mirrorDir = direction(reflektor.angle);
        const glm::vec2 half = mirrorDir * (kMirrorLength * 0.5f);
        const glm::vec2 a = reflektor.position - half;
        const glm::vec2 b = reflektor.position + half;
        const glm::vec2 seg = b - a;
        const float denominator = cross(ray.rayDir, seg);
        if (std::abs(denominator) < kEpsilon) {
          continue;
        }
        const glm::vec2 toSegment = a - ray.origin;
        const float rayDistance = cross(toSegment, seg) / denominator;
        const float segmentAmount = cross(toSegment, ray.rayDir) / denominator;
        if (rayDistance <= kEpsilon || segmentAmount < 0.0f ||
            segmentAmount > 1.0f) {
          continue;
        }
        Hit hit;
        hit.type = Hit::Type::Reflektor;
        hit.distance = rayDistance;
        hit.point = ray.origin + ray.rayDir * rayDistance;
        hit.mirrorDir = mirrorDir;
        hit.index = static_cast<int>(i);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      for (std::size_t i = 0; i < world.targets.size(); ++i) {
        const Target &target = world.targets[i];
        if (!target.alive) {
          continue;
        }
        const glm::vec2 toTarget = target.position - ray.origin;
        const float projected = glm::dot(toTarget, ray.rayDir);
        if (projected <= kEpsilon) {
          continue;
        }
        const glm::vec2 closest = ray.origin + ray.rayDir * projected;
        const float distanceToRay = glm::length(target.position - closest);
        if (distanceToRay > kTargetRadius) {
          continue;
        }
        const float hitDistance =
            projected - std::sqrt(kTargetRadius * kTargetRadius -
                                  distanceToRay * distanceToRay);
        if (hitDistance <= kEpsilon) {
          continue;
        }
        Hit hit;
        hit.type = Hit::Type::Target;
        hit.distance = hitDistance;
        hit.point = ray.origin + ray.rayDir * hitDistance;
        hit.index = static_cast<int>(i);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      for (std::size_t i = 0; i < world.portals.size(); ++i) {
        if (static_cast<int>(i) == ray.ignorePortal) {
          continue;
        }
        const Portal &portal = world.portals[i];
        const glm::vec2 toPortal = portal.entryPosition - ray.origin;
        const float projected = glm::dot(toPortal, ray.rayDir);
        if (projected <= kEpsilon) {
          continue;
        }
        const glm::vec2 closest = ray.origin + ray.rayDir * projected;
        const float distanceToRay = glm::length(portal.entryPosition - closest);
        const float portalRadius = 20.0f;
        if (distanceToRay > portalRadius) {
          continue;
        }
        const float hitDistance =
            projected -
            std::sqrt(portalRadius * portalRadius -
                      distanceToRay * distanceToRay);
        if (hitDistance <= kEpsilon) {
          continue;
        }
        Hit hit;
        hit.type = Hit::Type::Portal;
        hit.distance = hitDistance;
        hit.point = ray.origin + ray.rayDir * hitDistance;
        hit.index = static_cast<int>(i);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      for (std::size_t i = 0; i < world.filters.size(); ++i) {
        if (static_cast<int>(i) == ray.ignoreFilter) {
          continue;
        }
        const Filter &filter = world.filters[i];
        const float minX = filter.position.x - kFilterHalfSize;
        const float maxX = filter.position.x + kFilterHalfSize;
        const float minY = filter.position.y - kFilterHalfSize;
        const float maxY = filter.position.y + kFilterHalfSize;
        float tMin = -1000000.0f;
        float tMax = 1000000.0f;

        if (std::abs(ray.rayDir.x) < kEpsilon) {
          if (ray.origin.x < minX || ray.origin.x > maxX) {
            continue;
          }
        } else {
          const float tx1 = (minX - ray.origin.x) / ray.rayDir.x;
          const float tx2 = (maxX - ray.origin.x) / ray.rayDir.x;
          tMin = std::max(tMin, std::min(tx1, tx2));
          tMax = std::min(tMax, std::max(tx1, tx2));
        }

        if (std::abs(ray.rayDir.y) < kEpsilon) {
          if (ray.origin.y < minY || ray.origin.y > maxY) {
            continue;
          }
        } else {
          const float ty1 = (minY - ray.origin.y) / ray.rayDir.y;
          const float ty2 = (maxY - ray.origin.y) / ray.rayDir.y;
          tMin = std::max(tMin, std::min(ty1, ty2));
          tMax = std::min(tMax, std::max(ty1, ty2));
        }

        if (tMax < tMin || tMax <= kEpsilon) {
          continue;
        }

        const glm::vec2 filterDir = direction(filter.angle);
        const float alignment = std::abs(glm::dot(ray.rayDir, filterDir));
        Hit hit;
        hit.type = Hit::Type::Filter;
        hit.distance = std::max(tMin, kEpsilon);
        hit.exitDistance = std::max(tMax, hit.distance);
        hit.point = ray.origin + ray.rayDir * hit.distance;
        hit.index = static_cast<int>(i);
        hit.passesFilter = alignment >= std::cos(kFilterPassHalfAngle);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      for (std::size_t i = 0; i < world.splitters.size(); ++i) {
        if (static_cast<int>(i) == ray.ignoreSplitter) {
          continue;
        }
        const Splitter &splitter = world.splitters[i];
        const glm::vec2 toSplitter = splitter.position - ray.origin;
        const float projected = glm::dot(toSplitter, ray.rayDir);
        if (projected <= kEpsilon) {
          continue;
        }
        const glm::vec2 closest = ray.origin + ray.rayDir * projected;
        const float distanceToRay = glm::length(splitter.position - closest);
        if (distanceToRay > kSplitterRadius) {
          continue;
        }
        const float hitDistance =
            projected -
            std::sqrt(kSplitterRadius * kSplitterRadius -
                      distanceToRay * distanceToRay);
        if (hitDistance <= kEpsilon) {
          continue;
        }
        Hit hit;
        hit.type = Hit::Type::Splitter;
        hit.distance = hitDistance;
        hit.point = ray.origin + ray.rayDir * hitDistance;
        hit.index = static_cast<int>(i);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      for (std::size_t i = 0; i < world.blockers.size(); ++i) {
        if (static_cast<int>(i) == ray.ignoreBlocker) {
          continue;
        }
        const Blocker &blocker = world.blockers[i];
        const float minX = blocker.position.x - kBlockerHalfSize;
        const float maxX = blocker.position.x + kBlockerHalfSize;
        const float minY = blocker.position.y - kBlockerHalfSize;
        const float maxY = blocker.position.y + kBlockerHalfSize;
        float tMin = -1000000.0f;
        float tMax = 1000000.0f;
        bool hitAxisX = true;

        if (std::abs(ray.rayDir.x) < kEpsilon) {
          if (ray.origin.x < minX || ray.origin.x > maxX) {
            continue;
          }
        } else {
          const float tx1 = (minX - ray.origin.x) / ray.rayDir.x;
          const float tx2 = (maxX - ray.origin.x) / ray.rayDir.x;
          const float txMin = std::min(tx1, tx2);
          if (txMin > tMin) {
            tMin = txMin;
            hitAxisX = true;
          }
          tMax = std::min(tMax, std::max(tx1, tx2));
        }

        if (std::abs(ray.rayDir.y) < kEpsilon) {
          if (ray.origin.y < minY || ray.origin.y > maxY) {
            continue;
          }
        } else {
          const float ty1 = (minY - ray.origin.y) / ray.rayDir.y;
          const float ty2 = (maxY - ray.origin.y) / ray.rayDir.y;
          const float tyMin = std::min(ty1, ty2);
          if (tyMin > tMin) {
            tMin = tyMin;
            hitAxisX = false;
          }
          tMax = std::min(tMax, std::max(ty1, ty2));
        }

        if (tMax < tMin || tMax <= kEpsilon) {
          continue;
        }
        Hit hit;
        hit.type = Hit::Type::Blocker;
        hit.distance = std::max(tMin, kEpsilon);
        hit.point = ray.origin + ray.rayDir * hit.distance;
        hit.index = static_cast<int>(i);
        hit.normal = hitAxisX
                         ? glm::vec2(ray.rayDir.x > 0.0f ? -1.0f : 1.0f, 0.0f)
                         : glm::vec2(0.0f,
                                     ray.rayDir.y > 0.0f ? -1.0f : 1.0f);
        if (closer(hit, nearest)) {
          nearest = hit;
        }
      }

      if (nearest.type == Hit::Type::Reflektor) {
        const float stopGap = std::min(kHitGap, nearest.distance * 0.5f);
        const glm::vec2 reflected =
            reflectAcrossMirror(ray.rayDir, nearest.mirrorDir);
        result.activeReflektors[nearest.index] = true;
        result.reflektorEnergy[nearest.index] = ray.energy;
        addSegment(result, ray.visualOrigin,
                   nearest.point - ray.rayDir * stopGap, ray.energy,
                   maxSegments, BeamSegmentEnd::Reflektor);
        ray.origin = nearest.point + reflected * kHitGap;
        ray.visualOrigin = nearest.point;
        ray.rayDir = glm::normalize(reflected);
        ray.ignoreReflektor = nearest.index;
        ray.ignoreBlocker = -1;
        ray.ignorePortal = -1;
        ray.ignoreFilter = -1;
        ray.ignoreSplitter = -1;
        ray.energy = std::min(ray.energy + 1.0f, 3.0f);
      } else if (nearest.type == Hit::Type::Target) {
        addSegment(result, ray.visualOrigin, nearest.point, ray.energy,
                   maxSegments, BeamSegmentEnd::Target);
        result.hitTargets[nearest.index] = true;
        result.targetEnergy[nearest.index] = ray.energy;
        break;
      } else if (nearest.type == Hit::Type::Blocker) {
        const Blocker &blocker = world.blockers[nearest.index];
        addSegment(result, ray.visualOrigin, nearest.point, ray.energy,
                   maxSegments, BeamSegmentEnd::Blocker);
        result.activeBlockers[nearest.index] = true;
        result.blockerEnergy[nearest.index] = ray.energy;
        result.blockerHit[nearest.index] =
            (nearest.point - blocker.position) / kBlockerHalfSize;
        result.blockerHasHit[nearest.index] = true;
        if (blocker.reflective) {
          const glm::vec2 reflected =
              reflectFromNormal(ray.rayDir, nearest.normal);
          ray.origin = nearest.point + reflected * kHitGap;
          ray.visualOrigin = nearest.point;
          ray.rayDir = glm::normalize(reflected);
          ray.ignoreReflektor = -1;
          ray.ignoreBlocker = nearest.index;
          ray.ignorePortal = -1;
          ray.ignoreFilter = -1;
          ray.ignoreSplitter = -1;
        } else {
          break;
        }
      } else if (nearest.type == Hit::Type::Portal) {
        const Portal &portal = world.portals[nearest.index];
        addSegment(result, ray.visualOrigin, nearest.point, ray.energy,
                   maxSegments, BeamSegmentEnd::Portal);
        result.activePortals[nearest.index] = true;
        result.portalEntryHit[nearest.index] =
            (nearest.point - portal.entryPosition) / 20.0f;
        result.portalExitHit[nearest.index] = ray.rayDir;
        result.portalHasHit[nearest.index] = true;
        ray.origin = portal.exitPosition + ray.rayDir * kHitGap;
        ray.visualOrigin = portal.exitPosition;
        ray.ignoreReflektor = -1;
        ray.ignoreBlocker = -1;
        ray.ignorePortal = nearest.index;
        ray.ignoreFilter = -1;
        ray.ignoreSplitter = -1;
      } else if (nearest.type == Hit::Type::Filter) {
        const Filter &filter = world.filters[nearest.index];
        const glm::vec2 localHit =
            rotateVec(nearest.point - filter.position, -filter.angle);
        result.filterHit[nearest.index] = localHit / kFilterHalfSize;
        result.filterHasHit[nearest.index] = true;
        if (nearest.passesFilter) {
          const glm::vec2 exitPoint =
              ray.origin + ray.rayDir * nearest.exitDistance;
          for (std::size_t i = 0; i < world.targets.size(); ++i) {
            const Target &target = world.targets[i];
            if (!target.alive || result.hitTargets[i]) {
              continue;
            }
            const glm::vec2 toTarget = target.position - ray.origin;
            const float projected = glm::dot(toTarget, ray.rayDir);
            if (projected < nearest.distance - kEpsilon ||
                projected > nearest.exitDistance + kEpsilon) {
              continue;
            }
            const glm::vec2 closest = ray.origin + ray.rayDir * projected;
            if (glm::length(target.position - closest) <= kTargetRadius) {
              result.hitTargets[i] = true;
              result.targetEnergy[i] = ray.energy;
            }
          }
          addSegment(result, ray.visualOrigin, exitPoint, ray.energy,
                     maxSegments, BeamSegmentEnd::Filter);
          result.passingFilters[nearest.index] = true;
          ray.origin = exitPoint + ray.rayDir * kHitGap;
          ray.visualOrigin = exitPoint;
          ray.ignoreReflektor = -1;
          ray.ignoreBlocker = -1;
          ray.ignorePortal = -1;
          ray.ignoreFilter = nearest.index;
          ray.ignoreSplitter = -1;
        } else {
          addSegment(result, ray.visualOrigin, nearest.point, ray.energy,
                     maxSegments, BeamSegmentEnd::Filter);
          result.blockedFilters[nearest.index] = true;
          break;
        }
      } else if (nearest.type == Hit::Type::Splitter) {
        const Splitter &splitter = world.splitters[nearest.index];
        addSegment(result, ray.visualOrigin, nearest.point, ray.energy,
                   maxSegments, BeamSegmentEnd::Splitter);
        result.activeSplitters[nearest.index] = true;
        result.splitterHit[nearest.index] =
            rotateVec(nearest.point - splitter.position, -splitter.angle) /
            kSplitterRadius;
        result.splitterHasHit[nearest.index] = true;
        if (ray.depth < 3) {
          const float nextEnergy = std::min(ray.energy + 0.5f, 3.0f);
          const glm::vec2 branchA =
              glm::normalize(rotateVec(ray.rayDir, kSplitterBranchAngle));
          const glm::vec2 branchB =
              glm::normalize(rotateVec(ray.rayDir, -kSplitterBranchAngle));
          BeamRay a{nearest.point + branchA * kHitGap,
                    nearest.point,
                    branchA,
                    -1,
                    -1,
                    -1,
                    -1,
                    nearest.index,
                    nextEnergy,
                    ray.depth + 1};
          BeamRay b{nearest.point + branchB * kHitGap,
                    nearest.point,
                    branchB,
                    -1,
                    -1,
                    -1,
                    -1,
                    nearest.index,
                    nextEnergy,
                    ray.depth + 1};
          rays.push_back(b);
          rays.push_back(a);
        }
        break;
      } else {
        addSegment(result, ray.visualOrigin,
                   ray.visualOrigin + ray.rayDir * kBeamRange, ray.energy,
                   maxSegments);
        break;
      }
    }
  }

  return result;
}

} // namespace Deflektorish
