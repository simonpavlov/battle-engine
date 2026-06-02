#pragma once

#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionEvents.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct Engine;

struct IPositionSystem : ISystem {
    Signal<events::MapCreated>::Sink onMapCreated() {
        return mapCreated.sink();
    }

    Signal<events::MarchStarted>::Sink onMarchStarted() {
        return marchStarted.sink();
    }

    Signal<events::Moved>::Sink onMoved() {
        return moved.sink();
    }

    Signal<events::MarchEnded>::Sink onMarchEnded() {
        return marchEnded.sink();
    }

    virtual void setBounds(uint32_t width, uint32_t height) = 0;

    virtual components::Position getPosition(UnitId id) = 0;

    virtual bool move(UnitId unit_to_move_id, components::Position target_position) = 0;

    virtual std::vector<UnitId> unitsInRange(
        components::Position center, Distance min_range, Distance max_range, UnitId exclude
    ) = 0;

    virtual void march(UnitId id, components::Position target) = 0;

    virtual bool advanceMarch(UnitId id) = 0;

    ~IPositionSystem() override = default;

protected:
    Signal<events::MapCreated> mapCreated;
    Signal<events::MarchStarted> marchStarted;
    Signal<events::Moved> moved;
    Signal<events::MarchEnded> marchEnded;
};

using IPositionSystemPtr = std::unique_ptr<IPositionSystem>;

IPositionSystemPtr makeCorePositionSystem(Engine& engine);

}  // namespace sw::core
