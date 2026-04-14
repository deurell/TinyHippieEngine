#pragma once

#include "renderdevice.h"

namespace DL {

void applyDebugUiStyle();
void beginDebugUiFrame();
void endDebugUiFrame();
void drawFrameStatsOverlay(double frameTimeSeconds,
                           const RenderStats &renderStats);
void drawLogWindow();

} // namespace DL
