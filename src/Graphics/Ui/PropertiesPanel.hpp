#pragma once

#include <Core/Foundation/Engine.hpp>

#include "EventFeed.hpp"
#include "VisualizerState.hpp"
#include "WorldEditor.hpp"

namespace sw::graphics::ui {

void drawPropertiesPanel(sw::core::Engine& engine, VisualizerState& state, const EventFeed& feed, WorldEditor& editor);

}  // namespace sw::graphics::ui
