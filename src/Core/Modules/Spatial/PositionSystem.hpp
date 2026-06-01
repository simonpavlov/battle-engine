#pragma once

#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct Engine;

struct IPositionSystem : ISystem {
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

    virtual void setBounds(uint32_t width, uint32_t height) = 0;

    virtual Position getPosition(UnitId id) = 0;

    virtual bool move(UnitId unit_to_move_id, Position target_position) = 0;

    virtual std::vector<UnitId> unitsInRange(Position center, uint32_t min_range, uint32_t max_range, UnitId exclude)
        = 0;

    virtual void march(UnitId id, Position target) = 0;

    virtual bool advanceMarch(UnitId id) = 0;

    ~IPositionSystem() override = default;

protected:
    Signal<uint32_t, uint32_t> mapCreated;
    Signal<UnitId, Position, Position> marchStarted;
    Signal<UnitId, Position> moved;
    Signal<UnitId, Position> marchEnded;
};

using IPositionSystemPtr = std::unique_ptr<IPositionSystem>;

IPositionSystemPtr makeCorePositionSystem(Engine& engine);

}  // namespace sw::core
