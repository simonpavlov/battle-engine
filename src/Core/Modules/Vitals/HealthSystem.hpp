#pragma once

#include <Core/Foundation/Signal.hpp>
#include <Core/Foundation/Systems.hpp>
#include <Core/Foundation/Unit.hpp>

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

    ~IHealthSystem() override = default;

protected:
    // TODO: Signal-s should contains well-named structure parameters, like AttackedData {source, target, damage, target_hp}
    // align with this rule every Signal definition
    Signal<UnitId, UnitId, int, int> attacked;
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr makeCoreHealthSystem(Engine& engine);

}  // namespace sw::core
