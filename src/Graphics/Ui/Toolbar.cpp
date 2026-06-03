#include "Toolbar.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "rlImGui.h"

#include <Core/Foundation/UnitSystem.hpp>
#include <format>
#include <string>

namespace sw::graphics::ui {

namespace {

constexpr float kSpeedSliderWidth = 160.0F;
constexpr float kSpeedMin = 0.5F;
constexpr float kSpeedMax = 20.0F;
constexpr float kIconSize = 20.0F;

void tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }
}

bool actionButton(bool useIcons, const char* id, const Texture2D& texture, const char* label, float iconSize) {
    if (!useIcons) {
        return ImGui::Button(label);
    }
    const bool clicked = rlImGuiImageButtonSize(id, &texture, Vector2{iconSize, iconSize});
    tooltip(label);
    return clicked;
}

void toggleButton(bool useIcons, const char* id, const Texture2D& texture, const char* label, bool& flag, float iconSize) {
    const bool active = flag;
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (actionButton(useIcons, id, texture, label, iconSize)) {
        flag = !flag;
    }
    if (active) {
        ImGui::PopStyleColor();
    }
}

void drawLoadModal(VisualizerState& state) {
    if (!ImGui::BeginPopupModal("Load Scenario", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::InputText("path", &state.loadPath);
    if (ImGui::Button("Load##confirm")) {
        state.loadRequested = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

}  // namespace

void drawSpeedSlider(VisualizerState& state, float iconSize) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const float buttonHeight = iconSize + (style.FramePadding.y * 2.0F);
    const float padY = std::max((buttonHeight - ImGui::GetFontSize()) * 0.5F, style.FramePadding.y);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{style.FramePadding.x, padY});
    ImGui::SetNextItemWidth(kSpeedSliderWidth * state.uiScale);
    ImGui::SliderFloat("##speed", &state.speed, kSpeedMin, kSpeedMax, "%.1f tps");
    ImGui::PopStyleVar();
    tooltip("Speed (ticks/sec)");
}

void drawToolbar(sw::core::Engine& engine, VisualizerState& state, const IconSet& icons) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float iconSize = kIconSize * state.uiScale;
    const float barHeight = iconSize + (style.FramePadding.y * 2.0F) + (style.WindowPadding.y * 2.0F);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::BeginViewportSideBar("##Toolbar", viewport, ImGuiDir_Up, barHeight, flags)) {
        const bool useIcons = icons.loaded();

        if (actionButton(useIcons, "##load", icons.folder(), "Load", iconSize)) {
            ImGui::OpenPopup("Load Scenario");
        }
        drawLoadModal(state);

        ImGui::SameLine();
        ImGui::BeginDisabled(state.loadPath.empty());
        if (actionButton(useIcons, "##reload", icons.reload(), "Reload scenario", iconSize)) {
            state.loadRequested = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        const Texture2D& playPause = state.playing ? icons.pause() : icons.play();
        if (actionButton(useIcons, "##playpause", playPause, state.playing ? "Pause" : "Play", iconSize)) {
            state.playing = !state.playing;
        }

        ImGui::SameLine();
        if (actionButton(useIcons, "##step", icons.step(), "Step", iconSize)) {
            state.stepRequested = true;
        }

        ImGui::SameLine();
        drawSpeedSlider(state, iconSize);

        ImGui::SameLine();
        toggleButton(useIcons, "##units", icons.panelUnits(), "Units", state.showUnits, iconSize);
        ImGui::SameLine();
        toggleButton(useIcons, "##properties", icons.panelProperties(), "Properties", state.showProperties, iconSize);
        ImGui::SameLine();
        toggleButton(useIcons, "##log", icons.panelLog(), "Log", state.showLog, iconSize);

        const auto& units = engine.systems.getSystem<sw::core::IUnitSystem>();
        const std::string status = std::format("Tick {} \xC2\xB7 Alive {}", engine.tick, units.aliveCount());
        const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - statusWidth);
        ImGui::TextUnformatted(status.c_str());
    }
    ImGui::End();
}

}  // namespace sw::graphics::ui
