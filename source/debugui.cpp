#include "debugui.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace DL {

namespace {
bool frameStarted = false;
}

void beginDebugUiFrame() {
#ifdef USE_IMGUI
  if (frameStarted) {
    return;
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  frameStarted = true;
#endif
}

void endDebugUiFrame() {
#ifdef USE_IMGUI
  frameStarted = false;
#endif
}

void drawFrameStatsOverlay(double frameTimeSeconds,
                           const RenderStats &renderStats) {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  ImGui::SetNextWindowBgAlpha(0.92f);
  ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
  if (ImGui::Begin("Frame Stats", nullptr, flags)) {
    const float frameMs = static_cast<float>(frameTimeSeconds * 1000.0);
    const float fps =
        frameTimeSeconds > 0.0 ? static_cast<float>(1.0 / frameTimeSeconds)
                               : 0.0f;
    ImGui::Text("FPS %.1f", fps);
    ImGui::Text("Frame %.2f ms", frameMs);
    ImGui::Separator();
    ImGui::Text("Draw calls %u", renderStats.drawCalls);
    ImGui::Text("Triangles %u", renderStats.triangles);
    ImGui::Text("Cached meshes %u", renderStats.meshCount);
    ImGui::Text("Cached textures %u", renderStats.textureCount);
    ImGui::Text("Cached shader programs %u", renderStats.pipelineCount);
  }
  ImGui::End();
#endif
}

} // namespace DL
