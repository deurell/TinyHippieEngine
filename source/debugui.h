#pragma once

#include "renderdevice.h"

namespace DL {
class App;

void applyDebugUiStyle();
void beginDebugUiFrame();
void endDebugUiFrame();
void drawFrameStatsOverlay(double frameTimeSeconds,
                           const RenderStats &renderStats);
void drawEngineDebugWindows(App &app, double frameTimeSeconds,
                            const RenderStats &renderStats);
void drawLogWindow();

} // namespace DL
