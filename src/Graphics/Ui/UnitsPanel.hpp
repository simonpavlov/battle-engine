#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <optional>

#include "EventFeed.hpp"
#include "VisualizerState.hpp"
#include "WorldEditor.hpp"

namespace sw::graphics::ui {

void drawUnitsPanel(sw::core::Engine& engine, VisualizerState& state, const EventFeed& feed, WorldEditor& editor);

void applyAddForm(
    WorldEditor& editor,
    VisualizerState& state,
    sw::core::components::Position spawn,
    std::optional<sw::core::components::Position> destination
);

}  // namespace sw::graphics::ui
