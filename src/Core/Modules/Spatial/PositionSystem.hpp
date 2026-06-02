#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <Core/Modules/Spatial/PositionEvents.hpp>
#include <cstdint>
#include <vector>

namespace sw::core {

struct IPositionSystem : ISystem {
    Signal<events::MapCreated>::Sink onMapCreated() {
        return mapCreated_.sink();
    }

    Signal<events::MarchStarted>::Sink onMarchStarted() {
        return marchStarted_.sink();
    }

    Signal<events::Moved>::Sink onMoved() {
        return moved_.sink();
    }

    Signal<events::MarchEnded>::Sink onMarchEnded() {
        return marchEnded_.sink();
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
    Signal<events::MapCreated> mapCreated_;
    Signal<events::MarchStarted> marchStarted_;
    Signal<events::Moved> moved_;
    Signal<events::MarchEnded> marchEnded_;
};

using IPositionSystemPtr = std::unique_ptr<IPositionSystem>;

IPositionSystemPtr makeCorePositionSystem(ComponentsLocator& components, SystemsLocator& systems);

}  // namespace sw::core
