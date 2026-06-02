#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Foundation/UnitEvents.hpp>
#include <Core/Modules/Spatial/Position.hpp>
#include <cstddef>
#include <unordered_map>

namespace sw::core {

using UnitTypes = std::unordered_map<UnitTypeId, UnitType>;
using UnitToType = std::unordered_map<UnitId, UnitTypeRef>;

struct IUnitSystem : ISystem {
    Signal<events::Spawned>::Sink onSpawned() {
        return spawned_.sink();
    }

    Signal<events::Died>::Sink onDied() {
        return died_.sink();
    }

    virtual void registerUnitType(UnitType&& unit_type) = 0;
    virtual void addUnit(UnitTypeId unit_type_id, UnitId unit_id) = 0;

    virtual void scheduleDeath(UnitId unit_id) = 0;
    virtual void sweep() = 0;
    virtual bool stepActions() = 0;

    virtual std::size_t aliveCount() const = 0;
    virtual const UnitType& getUnitType(UnitId unit_id) const = 0;

    ~IUnitSystem() override = default;

protected:
    Signal<events::Spawned> spawned_;
    Signal<events::Died> died_;
};

using IUnitSystemPtr = std::unique_ptr<IUnitSystem>;

IUnitSystemPtr makeCoreUnitSystem(ComponentsLocator& components);

}  // namespace sw::core
