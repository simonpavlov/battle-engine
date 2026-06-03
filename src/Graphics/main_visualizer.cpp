#include "EngineBuilder.hpp"
#include "EventFeed.hpp"
#include "GraphicsSystem.hpp"
#include "IconSet.hpp"
#include "VisualizerState.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"

#include <Core/Foundation/Engine.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;
constexpr int kTargetFps = 60;

constexpr float kDefaultUiScale = 1.5F;
constexpr float kMinUiScale = 0.5F;
constexpr float kMaxUiScale = 4.0F;

std::optional<float> readUiScaleEnv() {
    const char* env = std::getenv("IMGUI_SCALE");
    if (env == nullptr) {
        return std::nullopt;
    }
    char* end = nullptr;
    const float parsed = std::strtof(env, &end);
    if (end == env || parsed <= 0.0F) {
        return std::nullopt;
    }
    return std::clamp(parsed, kMinUiScale, kMaxUiScale);
}

constexpr float kCameraZoomMin = 2.0F;
constexpr float kCameraZoomMax = 80.0F;
constexpr float kCameraZoomEpsilon = 0.0001F;

float cameraZoomDistance(const sw::graphics::VisualizerState& state) {
    const float dx = state.camera.position.x - state.camera.target.x;
    const float dy = state.camera.position.y - state.camera.target.y;
    const float dz = state.camera.position.z - state.camera.target.z;
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

void setCameraZoomDistance(sw::graphics::VisualizerState& state, float distance) {
    const float current = cameraZoomDistance(state);
    if (current < kCameraZoomEpsilon) {
        return;
    }
    const float scale = std::clamp(distance, kCameraZoomMin, kCameraZoomMax) / current;
    auto& position = state.camera.position;
    const auto& target = state.camera.target;
    position.x = target.x + ((position.x - target.x) * scale);
    position.y = target.y + ((position.y - target.y) * scale);
    position.z = target.z + ((position.z - target.z) * scale);
}

std::optional<float> parseFloatAfter(std::string_view line, std::string_view key) {
    if (!line.starts_with(key)) {
        return std::nullopt;
    }
    char* end = nullptr;
    const float value = std::strtof(line.data() + key.size(), &end);
    if (end == line.data() + key.size()) {
        return std::nullopt;
    }
    return value;
}

// Persists camera zoom (distance to target) and UI scale into imgui.ini via ImGui's settings handler.
void* visualizerSettingsReadOpen(ImGuiContext* /*ctx*/, ImGuiSettingsHandler* handler, const char* name) {
    return std::strcmp(name, "Data") == 0 ? handler->UserData : nullptr;
}

void visualizerSettingsReadLine(ImGuiContext* /*ctx*/, ImGuiSettingsHandler* /*handler*/, void* entry, const char* line) {
    auto& state = *static_cast<sw::graphics::VisualizerState*>(entry);
    const std::string_view view{line};
    if (const auto zoom = parseFloatAfter(view, "Zoom=")) {
        setCameraZoomDistance(state, *zoom);
    } else if (const auto scale = parseFloatAfter(view, "Scale="); scale && !state.uiScaleFromEnv) {
        state.uiScale = std::clamp(*scale, kMinUiScale, kMaxUiScale);
    }
}

// Reconciles the live style with the loaded UI scale. ScaleAllSizes is multiplicative, so we
// correct by the ratio against what was already applied at startup (tracked via FontGlobalScale).
void visualizerSettingsApplyAll(ImGuiContext* /*ctx*/, ImGuiSettingsHandler* handler) {
    const auto& state = *static_cast<const sw::graphics::VisualizerState*>(handler->UserData);
    const float applied = ImGui::GetIO().FontGlobalScale;
    if (applied > 0.0F && state.uiScale != applied) {
        ImGui::GetStyle().ScaleAllSizes(state.uiScale / applied);
        ImGui::GetIO().FontGlobalScale = state.uiScale;
    }
}

void visualizerSettingsWriteAll(ImGuiContext* /*ctx*/, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
    const auto& state = *static_cast<const sw::graphics::VisualizerState*>(handler->UserData);
    const std::string text = std::format(
        "[{}][Data]\nZoom={:.3f}\nScale={:.3f}\n\n", handler->TypeName, cameraZoomDistance(state), state.uiScale
    );
    buf->append(text.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    using namespace sw;

    InitWindow(kWindowWidth, kWindowHeight, "sw_visualizer");
    SetTargetFPS(kTargetFps);
    rlImGuiSetup(true);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    graphics::IconSet icons;
    icons.load(GetApplicationDirectory() + std::string("assets/"));

    graphics::VisualizerState state;
    const std::optional<float> uiScaleEnv = readUiScaleEnv();
    state.uiScale = uiScaleEnv.value_or(kDefaultUiScale);
    state.uiScaleFromEnv = uiScaleEnv.has_value();
    ImGui::GetStyle().ScaleAllSizes(state.uiScale);
    ImGui::GetIO().FontGlobalScale = state.uiScale;
    graphics::EventFeed feed;

    ImGuiSettingsHandler visualizerSettings;
    visualizerSettings.TypeName = "Visualizer";
    visualizerSettings.TypeHash = ImHashStr("Visualizer");
    visualizerSettings.ReadOpenFn = visualizerSettingsReadOpen;
    visualizerSettings.ReadLineFn = visualizerSettingsReadLine;
    visualizerSettings.ApplyAllFn = visualizerSettingsApplyAll;
    visualizerSettings.WriteAllFn = visualizerSettingsWriteAll;
    visualizerSettings.UserData = &state;
    ImGui::AddSettingsHandler(&visualizerSettings);

    if (argc >= 2) {
        state.loadPath = argv[1];
    }

    auto loadScenario = [&](core::Engine& eng, const std::string& path) {
        graphics::buildEngine(eng);
        feed.clear();
        feed.subscribe(eng);
        if (!path.empty()) {
            std::ifstream file(path);
            if (file) {
                graphics::parseScenario(eng, file);
            } else {
                std::cerr << "Warning: could not open scenario file - " << path << '\n';
            }
        }
        std::uint64_t maxId = 0;
        for (const auto& [id, entry] : feed.roster()) {
            (void)entry;
            maxId = std::max(maxId, static_cast<std::uint64_t>(id.value));
        }
        state.nextId = maxId + 1;
        graphics::frameCamera(state, feed.boardWidth(), feed.boardHeight());
    };

    core::Engine engine;
    loadScenario(engine, state.loadPath);
    engine.systems.registerSystem<graphics::IGraphicsSystem>(graphics::makeGraphicsSystem(engine, state, feed, icons));

    auto* gfx = &engine.systems.getSystem<graphics::IGraphicsSystem>();

    if (const char* ticksEnv = std::getenv("SW_RECORD_TICKS")) {
        char* end = nullptr;
        const long parsed = std::strtol(ticksEnv, &end, 10);
        const int ticks = (end != ticksEnv && parsed > 0) ? static_cast<int>(parsed) : 0;
        constexpr int kWarmupFrames = 8;
        for (int i = 0; i < kWarmupFrames; ++i) {
            gfx->frame();
        }
        for (int t = 0; t <= ticks; ++t) {
            const std::string idx = std::to_string(t);
            std::string name = "frame_";
            name.append(3 - std::min<std::size_t>(idx.size(), 3), '0');
            name.append(idx);
            name.append(".png");
            state.capturePath = name;
            gfx->frame();
            engine.step();
        }
        icons.unload();
        rlImGuiShutdown();
        CloseWindow();
        return 0;
    }

    while (!gfx->shouldClose()) {
        gfx->frame();
        if (state.loadRequested) {
            state.loadRequested = false;
            engine = core::Engine{};
            loadScenario(engine, state.loadPath);
            engine.systems.registerSystem<graphics::IGraphicsSystem>(
                graphics::makeGraphicsSystem(engine, state, feed, icons)
            );
            gfx = &engine.systems.getSystem<graphics::IGraphicsSystem>();
        }
    }

    ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
    icons.unload();
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
