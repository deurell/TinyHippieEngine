#include "debugui.h"
#include "logger.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace DL {

namespace {
bool frameStarted = false;

const char *levelLabel(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return "Trace";
  case LogLevel::Info:
    return "Info";
  case LogLevel::Warning:
    return "Warning";
  case LogLevel::Error:
    return "Error";
  }
  return "Info";
}

int levelIndex(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return 0;
  case LogLevel::Info:
    return 1;
  case LogLevel::Warning:
    return 2;
  case LogLevel::Error:
    return 3;
  }
  return 1;
}

LogLevel levelFromIndex(int index) {
  switch (index) {
  case 0:
    return LogLevel::Trace;
  case 1:
    return LogLevel::Info;
  case 2:
    return LogLevel::Warning;
  case 3:
    return LogLevel::Error;
  default:
    return LogLevel::Info;
  }
}
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

void drawLogWindow() {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  static bool visible = true;
  static bool autoScroll = true;
  static char filterBuffer[128] = "";

  if (!visible) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(620.0f, 280.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Logs", &visible)) {
    ImGui::End();
    return;
  }

  auto &logger = Logger::instance();
  bool enabled = logger.isEnabled();
  if (ImGui::Checkbox("Enabled", &enabled)) {
    logger.setEnabled(enabled);
  }
  ImGui::SameLine();
  bool consoleEcho = logger.isConsoleEchoEnabled();
  if (ImGui::Checkbox("Console echo", &consoleEcho)) {
    logger.setConsoleEchoEnabled(consoleEcho);
  }
  ImGui::SameLine();
  bool fileSink = logger.isFileSinkEnabled();
  if (ImGui::Checkbox("Persist to file", &fileSink)) {
    logger.setFileSinkEnabled(fileSink);
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    logger.clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &autoScroll);

  int minLevel = levelIndex(logger.minimumLevel());
  const char *levelLabels[] = {"Trace", "Info", "Warning", "Error"};
  ImGui::PushItemWidth(140.0f);
  if (ImGui::Combo("Min level", &minLevel, levelLabels,
                   IM_ARRAYSIZE(levelLabels))) {
    logger.setMinimumLevel(levelFromIndex(minLevel));
  }
  ImGui::PopItemWidth();
  ImGui::Text("File %s", logger.logFilePath().c_str());
  ImGui::PushItemWidth(240.0f);
  ImGui::InputText("Filter", filterBuffer, IM_ARRAYSIZE(filterBuffer));
  ImGui::PopItemWidth();
  ImGui::Separator();

  const std::string_view filterText(filterBuffer);
  const auto entries = logger.snapshot();
  ImGui::BeginChild("LogEntries");
  for (const auto &entry : entries) {
    if (!filterText.empty() &&
        entry.formatted.find(filterText) == std::string::npos &&
        entry.subsystem.find(filterText) == std::string::npos &&
        entry.message.find(filterText) == std::string::npos &&
        entry.detail.find(filterText) == std::string::npos) {
      continue;
    }
    ImGui::TextUnformatted(entry.formatted.c_str());
  }
  if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::EndChild();
  ImGui::End();
#endif
}

} // namespace DL
