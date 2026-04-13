#pragma once

#include "renderdevice.h"

namespace DL {

void beginDebugUiFrame();
void endDebugUiFrame();
void drawFrameStatsOverlay(double frameTimeSeconds,
                           const RenderStats &renderStats);

} // namespace DL
