#pragma once

#include <Core/Signal.hpp>
#include <Core/Systems.hpp>
#include <Core/Unit.hpp>

namespace sw::core {

struct Engine;

struct IHealthSystem : ISystem {
    Signal<UnitId, UnitId, int, int>::Sink onAttacked() {
        return attacked.sink();
    }

    virtual bool has(UnitId id) = 0;
    virtual int getHp(UnitId id) = 0;
    virtual void applyDamage(UnitId source, UnitId target, int amount) = 0;
    virtual bool isAlive(UnitId id) = 0;

    virtual ~IHealthSystem() = default;

protected:
    Signal<UnitId, UnitId, int, int> attacked;  // source, target, damage, target hp after (clamped to 0)
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr MakeCoreHealthSystem(Engine& engine);

}  // namespace sw::core
