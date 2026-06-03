#pragma once

#include <Core/Foundation/Engine.hpp>

#include "IconSet.hpp"
#include "VisualizerState.hpp"

namespace sw::graphics::ui {

void drawToolbar(sw::core::Engine& engine, VisualizerState& state, const IconSet& icons);

}  // namespace sw::graphics::ui
