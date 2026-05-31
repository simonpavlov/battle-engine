#pragma once

#include <Core/Systems.hpp>
#include <Core/Unit.hpp>
#include <vector>

namespace sw::core {

struct Engine;

// Owns the Health component: the only code permitted to read/mutate hp and to decide who is dead.
struct IHealthSystem : ISystem {
    virtual bool has(UnitId id) = 0;
    virtual int getHp(UnitId id) = 0;
    virtual void applyDamage(UnitId id, int amount) = 0;
    virtual bool isAlive(UnitId id) = 0;

    // Ids whose hp has dropped to <= 0. The Engine performs the structural removal at end of tick.
    virtual std::vector<UnitId> collectDead() = 0;

    virtual ~IHealthSystem() = default;
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr MakeCoreHealthSystem(Engine& engine);

}
