#include "PropertiesPanel.hpp"

#include <Core/Modules/Combat/Range.hpp>
#include <Core/Modules/Spatial/Destination.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionSystem.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <cstdint>
#include <format>

#include "imgui.h"

namespace sw::graphics::ui {

namespace {

std::uint32_t clampUnsigned(int value) {
    return value < 0 ? 0U : static_cast<std::uint32_t>(value);
}

}  // namespace

void drawPropertiesPanel(sw::core::Engine& engine, VisualizerState& state, const EventFeed& feed, WorldEditor& editor) {
    if (!state.showProperties) {
        return;
    }

    ImGui::Begin("Properties");

    if (!state.hasSelection) {
        ImGui::TextUnformatted("No unit selected.");
        ImGui::End();
        return;
    }

    const auto id = state.selected;

    ImGui::TextUnformatted(std::format("#{}", id.value).c_str());

    const auto& roster = feed.roster();
    if (const auto it = roster.find(id); it != roster.end()) {
        ImGui::TextUnformatted(std::format("Type: {}", it->second.type).c_str());
    }

    auto& position = engine.systems.getSystem<sw::core::IPositionSystem>();
    const auto pos = position.getPosition(id);
    int px = static_cast<int>(pos.x);
    int py = static_cast<int>(pos.y);
    if (ImGui::InputInt("x", &px)) {
        editor.moveUnit(id, sw::core::components::Position{clampUnsigned(px), clampUnsigned(py)});
    }
    if (ImGui::InputInt("y", &py)) {
        editor.moveUnit(id, sw::core::components::Position{clampUnsigned(px), clampUnsigned(py)});
    }

    auto& health = engine.components.getComponent<sw::core::components::Health>();
    if (health.has(id)) {
        const auto& h = health.get(id);
        int hp = h.hp.value;
        int maxHp = h.max_hp.value;
        if (ImGui::InputInt("hp", &hp)) {
            editor.setHealth(id, hp, maxHp);
        }
        if (ImGui::InputInt("max hp", &maxHp)) {
            editor.setHealth(id, hp, maxHp);
        }
    }

    auto& strength = engine.components.getComponent<sw::core::components::Strength>();
    if (strength.has(id)) {
        int value = strength.get(id).value;
        if (ImGui::InputInt("strength", &value)) {
            editor.setStrength(id, value);
        }
    }

    auto& agility = engine.components.getComponent<sw::core::components::Agility>();
    if (agility.has(id)) {
        int value = agility.get(id).value;
        if (ImGui::InputInt("agility", &value)) {
            editor.setAgility(id, value);
        }
    }

    auto& range = engine.components.getComponent<sw::core::components::Range>();
    if (range.has(id)) {
        int value = static_cast<int>(range.get(id).value);
        if (ImGui::InputInt("range", &value)) {
            editor.setRange(id, clampUnsigned(value));
        }
    }

    ImGui::Separator();
    auto& destinations = engine.components.getComponent<sw::core::components::Destination>();
    if (destinations.has(id) && destinations.get(id).active) {
        const auto& dest = destinations.get(id);
        ImGui::TextUnformatted(std::format("Destination: ({}, {})", dest.target.x, dest.target.y).c_str());
    } else {
        ImGui::TextUnformatted("Destination: none");
    }
    if (ImGui::Button("Set destination")) {
        state.placement = Placement::SetDestination;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear destination")) {
        editor.clearDestination(id);
    }

    ImGui::Separator();
    if (ImGui::Button("Delete")) {
        editor.deleteUnit(id);
        state.hasSelection = false;
    }

    ImGui::End();
}

}  // namespace sw::graphics::ui
