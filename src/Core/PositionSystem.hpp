#pragma once

#include <Core/Position.hpp>
#include <Core/Signal.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct Engine;

struct IPositionSystem : ISystem {
    // Spatial-narrative events. The system owns and emits them; consumers only connect, via handles.
    Signal<uint32_t, uint32_t>::Sink onMapCreated() {
        return mapCreated.sink();
    }

    Signal<UnitId, Position, Position>::Sink onMarchStarted() {
        return marchStarted.sink();
    }

    Signal<UnitId, Position>::Sink onMoved() {
        return moved.sink();
    }

    Signal<UnitId, Position>::Sink onMarchEnded() {
        return marchEnded.sink();
    }

    // Map bounds; moves outside [0,width) x [0,height) are rejected.
    virtual void setBounds(uint32_t width, uint32_t height) = 0;

    virtual Position getPosition(UnitId id) = 0;

    // Move a unit onto target_position. Returns true iff the unit actually moved (the cell is in
    // bounds and not blocked by an occupant whose ICollisionReaction restricts it).
    virtual bool move(UnitId unit_to_move_id, Position target_position) = 0;

    // Unit ids whose Chebyshev distance from center is in [min_range, max_range], excluding `exclude`.
    virtual std::vector<UnitId> unitsInRange(Position center, uint32_t min_range, uint32_t max_range, UnitId exclude)
        = 0;

    // Begin a MARCH order toward target: emits marchStarted and records the destination.
    virtual void march(UnitId id, Position target) = 0;

    // Advance an active MARCH order by one step. Emits moved on a step and marchEnded on arrival;
    // returns true iff the unit actually moved this turn (drives run() termination).
    virtual bool advanceMarch(UnitId id) = 0;

    virtual ~IPositionSystem() = default;

protected:
    Signal<uint32_t, uint32_t> mapCreated;            // width, height
    Signal<UnitId, Position, Position> marchStarted;  // id, from, to
    Signal<UnitId, Position> moved;                   // id, to
    Signal<UnitId, Position> marchEnded;              // id, at
};

using IPositionSystemPtr = std::unique_ptr<IPositionSystem>;

IPositionSystemPtr MakeCorePositionSystem(Engine& engine);

}  // namespace sw::core
