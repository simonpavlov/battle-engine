#pragma once

#include <Core/Systems.hpp>
#include <Core/Unit.hpp>

namespace sw::core {

struct Engine;

struct IHealthSystem : ISystem {
    virtual bool has(UnitId id) = 0;
    virtual int getHp(UnitId id) = 0;
    virtual void applyDamage(UnitId id, int amount) = 0;
    virtual bool isAlive(UnitId id) = 0;

    virtual ~IHealthSystem() = default;
};

using IHealthSystemPtr = std::unique_ptr<IHealthSystem>;

IHealthSystemPtr MakeCoreHealthSystem(Engine& engine);

}  // namespace sw::core
