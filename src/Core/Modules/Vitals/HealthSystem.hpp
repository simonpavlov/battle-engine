#pragma once

#include <Core/Foundation/Components.hpp>
#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>
#include <Core/Modules/Vitals/HealthEvents.hpp>
#include <cstdint>

namespace sw::core {

struct IHealthSystem : ISystem {
    Signal<events::Attacked>::Sink onAttacked() {
        return attacked_.sink();
    }

    virtual bool has(UnitId id) = 0;
    virtual std::int32_t getHp(UnitId id) = 0;
    // TODO: should be applyDamage the part of IHealthSystem?
    virtual void applyDamage(UnitId source, UnitId target, std::int32_t amount) = 0;
    virtual bool isAlive(UnitId id) = 0;

    ~IHealthSystem() override = default;

protected:
    Signal<events::Attacked> attacked_;
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr makeCoreHealthSystem(ComponentsLocator& components, SystemsLocator& systems);

}  // namespace sw::core
