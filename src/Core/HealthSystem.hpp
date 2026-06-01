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
    // TODO: Signal-s should contains well-named structure parameters, like AttackedData {source, target, damage, target_hp}
    // align with this rule every Signal definition
    Signal<UnitId, UnitId, int, int> attacked;
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr makeCoreHealthSystem(Engine& engine);

}  // namespace sw::core
