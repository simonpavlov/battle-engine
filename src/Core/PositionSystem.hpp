#pragma once

#include <Core/Position.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct Engine;

struct IPositionSystem : ISystem {
    // Map bounds; moves outside [0,width) x [0,height) are rejected.
    virtual void setBounds(uint32_t width, uint32_t height) = 0;

    virtual Position getPosition(UnitId id) = 0;

    // Move a unit onto target_position. Returns true iff the unit actually moved (the cell is in
    // bounds and not blocked by an occupant whose ICollisionReaction restricts it).
    virtual bool move(UnitId unit_to_move_id, Position target_position) = 0;

    // Unit ids whose Chebyshev distance from center is in [min_range, max_range], excluding `exclude`.
    virtual std::vector<UnitId> unitsInRange(Position center, uint32_t min_range, uint32_t max_range,
                                             UnitId exclude) = 0;

    virtual ~IPositionSystem() = default;
};

using IPositionSystemPtr = std::unique_ptr<IPositionSystem>;

IPositionSystemPtr MakeCorePositionSystem(Engine& engine);

}
