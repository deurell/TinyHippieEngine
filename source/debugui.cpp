#include "debugui.h"
#include "app.h"
#include "logger.h"
#include "scenenode.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

namespace DL {

namespace {
bool frameStarted = false;
constexpr float kOverlayAlpha = 0.72f;
SceneNode *selectedNode = nullptr;
SceneNode *rotationEditorNode = nullptr;
glm::vec3 rotationEditorEulerDegrees(0.0f);

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

bool nodeExistsInSubtree(SceneNode *root, const SceneNode *candidate) {
  if (root == nullptr || candidate == nullptr) {
    return false;
  }
  if (root == candidate) {
    return true;
  }
  for (const auto &child : root->children) {
    if (nodeExistsInSubtree(child.get(), candidate)) {
      return true;
    }
  }
  return false;
}

const char *nodeLabel(SceneNode &node, std::size_t index) {
  if (!node.getDebugName().empty() && node.getDebugName() != "SceneNode") {
    return node.getDebugName().data();
  }
  if (!node.hasParent()) {
    return node.debugTypeName().data();
  }
  if (node.debugTypeName() != "SceneNode") {
    return node.debugTypeName().data();
  }
  thread_local std::string label;
  label = "Node ";
  label += std::to_string(index);
  return label.c_str();
}

void drawSceneNodeTree(SceneNode &node, std::size_t index = 0) {
  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
      ImGuiTreeNodeFlags_SpanAvailWidth;
  if (node.children.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (selectedNode == &node) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  const bool open =
      ImGui::TreeNodeEx(static_cast<void *>(&node), flags, "%s", nodeLabel(node, index));
  if (ImGui::IsItemClicked()) {
    selectedNode = &node;
  }

  if (open) {
    for (std::size_t childIndex = 0; childIndex < node.children.size(); ++childIndex) {
      drawSceneNodeTree(*node.children[childIndex], childIndex);
    }
    ImGui::TreePop();
  }
}

void drawNodeInspector(SceneNode &node) {
  glm::vec3 localPosition = node.getLocalPosition();
  glm::vec3 localScale = node.getLocalScale();
  if (rotationEditorNode != &node) {
    rotationEditorNode = &node;
    rotationEditorEulerDegrees =
        glm::degrees(glm::eulerAngles(node.getLocalRotation()));
  }

  ImGui::Text("Label %s", nodeLabel(node, 0));
  ImGui::Text("Children %zu", node.children.size());
  ImGui::Text("Render components %zu", node.renderComponentCount());
  bool debugOverride = node.isDebugTransformOverrideEnabled();
  if (ImGui::Checkbox("Override animated transform", &debugOverride)) {
    node.setDebugTransformOverrideEnabled(debugOverride);
  }
  ImGui::Separator();

  if (ImGui::DragFloat3("Local position", glm::value_ptr(localPosition), 0.05f)) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalPosition(localPosition);
    } else {
      node.setLocalPosition(localPosition);
    }
  }
  if (ImGui::DragFloat3("Local rotation", glm::value_ptr(rotationEditorEulerDegrees),
                        0.5f)) {
    const glm::quat rotation = glm::quat(glm::radians(rotationEditorEulerDegrees));
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalRotation(rotation);
    } else {
      node.setLocalRotation(rotation);
    }
  }
  if (ImGui::DragFloat3("Local scale", glm::value_ptr(localScale), 0.05f, 0.001f, 100.0f)) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalScale(localScale);
    } else {
      node.setLocalScale(localScale);
    }
  }

  if (ImGui::Button("Reset transform")) {
    if (node.isDebugTransformOverrideEnabled()) {
      node.setDebugLocalPosition(glm::vec3(0.0f));
      node.setDebugLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
      node.setDebugLocalScale(glm::vec3(1.0f));
    } else {
      node.setLocalPosition(glm::vec3(0.0f));
      node.setLocalRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
      node.setLocalScale(glm::vec3(1.0f));
    }
    rotationEditorEulerDegrees = glm::vec3(0.0f);
  }

  const glm::vec3 worldPosition = node.getWorldPosition();
  const glm::vec3 worldScale = node.getWorldScale();
  const glm::vec3 worldRotationEuler =
      glm::degrees(glm::eulerAngles(node.getWorldRotation()));

  ImGui::Separator();
  ImGui::Text("World position %.2f %.2f %.2f", worldPosition.x, worldPosition.y,
              worldPosition.z);
  ImGui::Text("World rotation %.1f %.1f %.1f", worldRotationEuler.x,
              worldRotationEuler.y, worldRotationEuler.z);
  ImGui::Text("World scale %.2f %.2f %.2f", worldScale.x, worldScale.y,
              worldScale.z);

  if (node.renderComponentCount() > 0) {
    ImGui::Separator();
    ImGui::TextUnformatted("Components");
    for (const auto &component : node.renderComponents()) {
      const std::string_view typeName = component->debugTypeName();
      ImGui::BulletText("%.*s", static_cast<int>(typeName.size()),
                        typeName.data());
    }
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

void drawEngineDebugWindows(App &app, double frameTimeSeconds,
                            const RenderStats &renderStats) {
#ifdef USE_IMGUI
  beginDebugUiFrame();

  auto *rootNode = dynamic_cast<SceneNode *>(app.currentScene());
  if (selectedNode != nullptr &&
      (rootNode == nullptr || !nodeExistsInSubtree(rootNode, selectedNode))) {
    selectedNode = rootNode;
  } else if (selectedNode == nullptr) {
    selectedNode = rootNode;
  }

  ImGui::SetNextWindowSize(ImVec2(310.0f, 260.0f), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Engine")) {
    const float frameMs = static_cast<float>(frameTimeSeconds * 1000.0);
    const float fps =
        frameTimeSeconds > 0.0 ? static_cast<float>(1.0 / frameTimeSeconds)
                               : 0.0f;
    constexpr ImGuiTreeNodeFlags sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;
    const glm::vec2 windowSize = app.windowSize();
    const glm::vec2 framebufferSize = app.framebufferSize();

    if (ImGui::CollapsingHeader("Timing", sectionFlags)) {
      bool paused = app.simulationPaused();
      if (ImGui::Checkbox("Pause fixed step", &paused)) {
        app.setSimulationPaused(paused);
      }
      ImGui::SameLine();
      if (ImGui::Button("Step")) {
        app.requestSimulationStep();
      }
      ImGui::Text("FPS %.1f", fps);
      ImGui::Text("Frame %.2f ms", frameMs);
      ImGui::Text("Fixed step %.2f ms", app.fixedTimeStep() * 1000.0f);
      ImGui::Text("Fixed updates last frame %d", app.lastFixedUpdateCount());
    }

    if (ImGui::CollapsingHeader("Window", sectionFlags)) {
      ImGui::Text("Window %.0f x %.0f", windowSize.x, windowSize.y);
      ImGui::Text("Framebuffer %.0f x %.0f", framebufferSize.x,
                  framebufferSize.y);
    }

    if (ImGui::CollapsingHeader("Rendering", sectionFlags)) {
      ImGui::Text("Draw calls %u", renderStats.drawCalls);
      ImGui::Text("Triangles %u", renderStats.triangles);
      ImGui::Text("Meshes %u", renderStats.meshCount);
      ImGui::Text("Textures %u", renderStats.textureCount);
      ImGui::Text("Pipelines %u", renderStats.pipelineCount);
    }

    if (ImGui::CollapsingHeader("Audio", sectionFlags)) {
      ImGui::Text("Audio clips %zu", app.audioSystem().loadedClipCount());
      ImGui::Text("Audio sounds %zu", app.audioSystem().activeSoundCount());
      float masterVolume = app.audioSystem().masterVolume();
      if (ImGui::SliderFloat("Master volume", &masterVolume, 0.0f, 2.0f,
                             "%.2f")) {
        app.audioSystem().setMasterVolume(masterVolume);
      }
      ImGui::TextUnformatted("Audio groups");
      for (int i = 0; i < static_cast<int>(DL::AudioGroup::Count); ++i) {
        const auto group = static_cast<DL::AudioGroup>(i);
        ImGui::PushID(i);
        bool muted = app.audioSystem().isGroupMuted(group);
        if (ImGui::Checkbox("Mute", &muted)) {
          app.audioSystem().setGroupMuted(group, muted);
        }
        ImGui::SameLine();
        ImGui::Text("%s (%zu)", DL::AudioSystem::groupLabel(group),
                    app.audioSystem().activeSoundCount(group));
        float groupVolume = app.audioSystem().groupVolume(group);
        if (ImGui::SliderFloat("Volume", &groupVolume, 0.0f, 2.0f, "%.2f")) {
          app.audioSystem().setGroupVolume(group, groupVolume);
        }
        ImGui::PopID();
      }
    }
  }
  ImGui::End();

  if (rootNode != nullptr) {
    ImGui::SetNextWindowSize(ImVec2(280.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Tree")) {
      drawSceneNodeTree(*rootNode);
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(360.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Inspector")) {
      if (selectedNode != nullptr) {
        drawNodeInspector(*selectedNode);
      } else {
        ImGui::TextUnformatted("No node selected.");
      }
    }
    ImGui::End();
  }
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
