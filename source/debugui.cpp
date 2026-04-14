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
constexpr float kOverlayAlpha = 0.72f;

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

void applyDebugUiStyle() {
#ifdef USE_IMGUI
  ImGui::StyleColorsLight();

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 10.0f;
  style.ChildRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.PopupRounding = 8.0f;
  style.GrabRounding = 6.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.98f, 0.99f, kOverlayAlpha);
  colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.18f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.99f, 0.90f);
  colors[ImGuiCol_Border] = ImVec4(0.68f, 0.74f, 0.80f, 0.55f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.89f, 0.92f, 0.96f, 0.76f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.88f, 0.95f, 0.88f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.92f, 0.95f, 0.98f, 0.80f);
  colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.68f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.88f, 0.96f, 0.78f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.74f, 0.84f, 0.95f, 0.86f);
  colors[ImGuiCol_Header] = ImVec4(0.70f, 0.82f, 0.94f, 0.42f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.55f, 0.75f, 0.95f, 0.66f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.71f, 0.94f, 0.82f);
  colors[ImGuiCol_Button] = ImVec4(0.52f, 0.74f, 0.96f, 0.54f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.67f, 0.94f, 0.78f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.61f, 0.90f, 0.88f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.86f, 0.90f, 0.95f, 0.30f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.56f, 0.67f, 0.78f, 0.55f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.46f, 0.60f, 0.76f, 0.72f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.54f, 0.72f, 0.86f);
  colors[ImGuiCol_Separator] = ImVec4(0.67f, 0.73f, 0.80f, 0.45f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.49f, 0.67f, 0.88f, 0.28f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.41f, 0.63f, 0.89f, 0.58f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.58f, 0.85f, 0.80f);
#endif
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

  ImGui::SetNextWindowBgAlpha(kOverlayAlpha);
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

  ImGui::SetNextWindowBgAlpha(kOverlayAlpha);
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
