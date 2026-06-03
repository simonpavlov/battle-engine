#pragma once

#include <Core/Foundation/Engine.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstdint>
#include <optional>

#include "EventFeed.hpp"
#include "raylib.h"

namespace sw::graphics {

std::optional<sw::core::UnitId> pickUnit(
    sw::core::Engine& engine, const EventFeed& feed, const Camera3D& camera, Vector2 mouse
);

std::optional<sw::core::components::Position> pickCell(
    const Camera3D& camera, Vector2 mouse, std::uint32_t boardWidth, std::uint32_t boardHeight
);

}  // namespace sw::graphics
