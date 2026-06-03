#include "UnitsPanel.hpp"

#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Combat/Range.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Stats/Agility.hpp>
#include <Core/Modules/Stats/Strength.hpp>
#include <Core/Modules/Vitals/Health.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "imgui.h"

namespace sw::graphics::ui {

namespace {

std::uint32_t clampUnsigned(int value) {
    return value < 0 ? 0U : static_cast<std::uint32_t>(value);
}

}  // namespace

void drawUnitsPanel(sw::core::Engine& engine, VisualizerState& state, const EventFeed& feed, WorldEditor& editor) {
    (void)engine;
    (void)editor;
    if (!state.showUnits) {
        return;
    }

    ImGui::Begin("Units");

    std::vector<sw::core::UnitId> ids;
    ids.reserve(feed.roster().size());
    for (const auto& [id, entry] : feed.roster()) {
        (void)entry;
        ids.push_back(id);
    }
    std::ranges::sort(ids, [](sw::core::UnitId lhs, sw::core::UnitId rhs) {
        return lhs.value < rhs.value;
    });

    for (const auto id : ids) {
        const auto& entry = feed.roster().at(id);
        const std::string label = std::format("#{} {}", id.value, entry.type);

        const bool selected = state.hasSelection && state.selected == id;
        if (ImGui::Selectable(label.c_str(), selected)) {
            state.selected = id;
            state.hasSelection = true;
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Add Unit");

    auto& form = state.addForm;
    static constexpr std::array<const char*, 3> kTypes = {"Swordsman", "Hunter", "Raven"};
    ImGui::Combo("Type", &form.typeIndex, kTypes.data(), static_cast<int>(kTypes.size()));
    ImGui::InputInt("hp", &form.hp);

    const bool isSwordsman = form.typeIndex == 0;
    const bool isHunter = form.typeIndex == 1;
    const bool isRaven = form.typeIndex == 2;

    if (isSwordsman || isHunter) {
        ImGui::InputInt("strength", &form.strength);
    }
    if (isHunter || isRaven) {
        ImGui::InputInt("agility", &form.agility);
    }
    if (isHunter) {
        ImGui::InputInt("range", &form.range);
    }

    if (state.placement == Placement::AddPosition) {
        ImGui::TextUnformatted("Click map: pick spawn cell");
    } else if (state.placement == Placement::AddDestination) {
        ImGui::TextUnformatted("Click map: pick destination (right-click/Esc to skip)");
    } else if (ImGui::Button("Add")) {
        state.placement = Placement::AddPosition;
        state.pendingPosition.reset();
    }

    ImGui::End();
}

void applyAddForm(
    WorldEditor& editor,
    VisualizerState& state,
    sw::core::components::Position spawn,
    std::optional<sw::core::components::Position> destination
) {
    const auto& form = state.addForm;
    const sw::core::UnitId id{static_cast<std::uint32_t>(state.nextId)};
    ++state.nextId;

    const bool isSwordsman = form.typeIndex == 0;
    const bool isHunter = form.typeIndex == 1;

    if (isSwordsman) {
        editor.spawnSwordsman(sw::feature::commands::SpawnSwordsman{
            id,
            spawn,
            sw::core::components::HealthPoints{form.hp},
            sw::core::components::Strength{form.strength},
        });
    } else if (isHunter) {
        editor.spawnHunter(sw::feature::commands::SpawnHunter{
            id,
            spawn,
            sw::core::components::HealthPoints{form.hp},
            sw::core::components::Agility{form.agility},
            sw::core::components::Strength{form.strength},
            sw::core::components::Range{clampUnsigned(form.range)},
        });
    } else {
        editor.spawnRaven(sw::feature::commands::SpawnRaven{
            id,
            spawn,
            sw::core::components::HealthPoints{form.hp},
            sw::core::components::Agility{form.agility},
        });
    }

    if (destination) {
        editor.setDestination(id, *destination);
    }
}

}  // namespace sw::graphics::ui
