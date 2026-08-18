#pragma once

namespace Deflektorish {

enum class ShaderStyle : int {
  Beam = 1,
  Source = 2,
  Target = 3,
  Blocker = 4,
  ReflectiveBlocker = 5,
  ManualReflector = 6,
  AutoReflector = 7,
  Selection = 8,
  Explosion = 9,
  Portal = 10,
  Filter = 11,
  Splitter = 12,
  EnergyBar = 13,
  CompletionOverlay = 14,
  FadeTransition = 15,
  ParallaxBackground = 16,
  EnemyCrawler = 17,
  EnemyNest = 18,
};

inline int shaderStyle(ShaderStyle style) { return static_cast<int>(style); }

} // namespace Deflektorish
